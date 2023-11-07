#include "http_conn11.h"
#include "../timer/Utils.h"



const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";
std::unordered_map<std::string,http_conn::METHOD> http_conn::methodMap;
std::map<std::string,std::string> users;

void http_conn::initmysql_result(connection_pool *connPool)
{
/*下面对users的操作不用加锁，因为这个函数仅执行一次*/
    auto mysql = connPool->GetConnection();
    if(mysql_query(mysql.get(),"select username,passwd from user"))
        LOG_ERROR("select error:%s",mysql_error(mysql.get()));
    MYSQL_RES *result = mysql_store_result(mysql.get());

    int num_fields = mysql_num_fields(result);

    MYSQL_FIELD *fileds = mysql_fetch_field(result);

    while(MYSQL_ROW row = mysql_fetch_row(result))
    {
        std::string temp1(row[0]);
        std::string temp2(row[1]);
        users[temp1] = temp2;
    }
}
/*这个函数别忘记添加*/
void http_conn::initMethodMap()
{
    methodMap["GET"] = METHOD::GET;
    methodMap["POST"] = METHOD::POST;
    methodMap["HEAD"] = METHOD::HEAD;
    methodMap["PUT"] = METHOD::PUT;
    methodMap["DELETE"] = METHOD::DELETE;
    methodMap["TRACE"] = METHOD::TRACE;
    methodMap["OPTIONS"] = METHOD::OPTIONS;
    methodMap["CONNECT"] = METHOD::CONNECT;
    methodMap["PATH"] = METHOD::PATH;
}

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;
int http_conn::m_close_log = 0;

void http_conn::close_conn(bool real_close)
{
    if(real_close && (m_sockfd != -1))
    {
        printf("close %d\n",m_sockfd);
        Utils_fd::removefd(m_epollfd,m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

void http_conn::init(int sockfd,const sockaddr_in &addr,char *root,int TRIGMode,
                        int close_log,std::string user,std::string passwd,std::string sqlname)
{
    m_sockfd = sockfd;
    m_address = addr;
    /*这个模式的设置应该再前面的吧*/
    m_TRIGMode = TRIGMode;
    Utils_fd::addfd(m_epollfd,sockfd,true,m_TRIGMode);
    m_user_count++;
    doc_root = root;
    m_close_log = close_log;

    init();
}

void http_conn::init()
{
    mysql = nullptr;
    bytes_to_send = 0;
    bytes_have_send = 0;
    m_check_state = CHECK_STATE::CHECK_STATE_REQUESTLINE;
    m_linger = false;
    m_method = METHOD::GET;
    // m_url = 0;
    // m_version = 0;
    m_content_length = 0;
    // m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    cgi = 0;
    m_state = 0;
    timer_flag = 0;
    improv = 0;

    memset(m_read_buf,'\0',READ_BUFFER_SIZE);
    memset(m_write_buf,'\0',WRITE_BUFFER_SIZE);
    // memset(m_real_file,'\0',FILENAME_LEN);
}

http_conn::LINE_STATUS http_conn::parse_line()//这个解析行  在linux高性能服务器的8.6 里面有注释为啥这样返回状态
{
    //m_read_idx 在read_once中改变了
    /*buf地址是连续的 因此可以做指针运算*/
    /*下面的代码是可行的  直接采用strpbrk函数
    就可以不用一个字符一个字符的遍历了*/
    char *target = strpbrk(m_read_buf+m_checked_idx, "\n\r");
    if(target == NULL)
        return LINE_STATUS::LINE_OPEN;
    else
    {
        // m_checked_idx = m_checked_idx + (target - (m_read_buf+m_checked_idx))
        m_checked_idx = static_cast<int>(target - m_read_buf);
        if(*target == '\r')
        {
            if ((m_checked_idx + 1) == m_read_idx)//这样说明 读的行不完整，想要分析需要进一步读取
                return LINE_STATUS::LINE_OPEN;
            else if (m_read_buf[m_checked_idx + 1] == '\n')//这个说明读了一行
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_STATUS::LINE_OK;
            }
            return LINE_STATUS::LINE_BAD;
        }
        else if (*target == '\n')
        {
            if (m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r')//这种情况也是独立一行
            {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_STATUS::LINE_OK;
            }
            return LINE_STATUS::LINE_BAD;
        }
    }
}
/*这里出现读取失败的话
int timer_flag;
int improv;
这两个参数会被设置
会致使主线程直接释放连接*/

/*擦  看见这些判断就有点恶心 想改了它
想让其判断 执行在编译期*/
bool http_conn::read_once()
{
    if(m_read_idx >= READ_BUFFER_SIZE)
        return false;
    int bytes_read = 0;

    if(m_TRIGMode == 0)
    {
        bytes_read = recv(m_sockfd,m_read_buf+m_read_idx,READ_BUFFER_SIZE-m_read_idx,0);
        
        /*应该先判断是否正确读取了*/
        if(bytes_read <= 0)
            return false;
        m_read_idx += bytes_read;
        return true;
    }
    else
    {
        while(true)
        {
            bytes_read = recv(m_sockfd,m_read_buf+m_read_idx,READ_BUFFER_SIZE-m_read_idx,0);
            if(bytes_read == -1)
            {
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            }
            else if(bytes_read == 0)
                return false;
            /*-1的时候不会走到这里的*/
            m_read_idx += bytes_read;
        }
        return true;
    }
}
/*这下面和牛客上的一模一样 不再重写*/
//解析http请求行，获得请求方法，目标url及http版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char *text)//这个也在8.6
{
    std::cmatch m;
    reg.assign("GET|POST|HEAD|PUT|DELETE|TRACE|OPTIONS|CONNECT|PATH");
    if(std::regex_search(text,m,reg) == 1)
    {
        m_method = methodMap[m.str()];
        cgi = static_cast<int>(m_method);
    }
    else
        return HTTP_CODE::BAD_REQUEST;
    reg.assign("HTTP/\\d.\\d");
    if(std::regex_search(text,m,reg) == 1)
    {
        m_version = m.str();
        if(m_version != "HTTP/1.1")
            return HTTP_CODE::BAD_REQUEST;
    }
    else
        return HTTP_CODE::BAD_REQUEST;
    reg.assign("(/.+)*(/.*) ");
    if(std::regex_search(text,m,reg) == 1)
    {
        m_url = m[2].str();
    }
    else
        return HTTP_CODE::BAD_REQUEST;
    //当url为/时，显示判断界面
    if ((m_url.size()) == 1)
        m_url += "judge.html";
    m_check_state = CHECK_STATE::CHECK_STATE_HEADER;
    return HTTP_CODE::NO_REQUEST;
}

//解析http请求的一个头部信息
http_conn::HTTP_CODE http_conn::parse_headers(char *text)
{
    if (text[0] == '\0')
    {
        if (m_content_length != 0)//这个表示  请求头处理完了  但是还有内容没有读，就变换状态 去读内容
        {
            m_check_state = CHECK_STATE::CHECK_STATE_CONTENT;
            return HTTP_CODE::NO_REQUEST;
        }
        return HTTP_CODE::GET_REQUEST;//这个标识读完头了 没有内容要读
    }
    std::cmatch m;
    reg.assign("(Connection:) *(keep-alive)|(Content-Length:) *(\\d+)|(Host:) *(.+:\\d+)");
    if(std::regex_search(text,m,reg))
    {
        if(m[1].str() == "Connection:")
        {
            if(m[2].str() == "keep-alive")
                m_linger = true;
        }
        else if(m[3].str() == "Content-Length:")
        {
            m_content_length = atol(m[4].str().c_str());
        }
        else if(m[5].str() == "Host:")
        {
            m_host = m[6].str();
        }
        else
        {
            LOG_INFO("oop!unknow header: %s", text);
        }
    }
    return HTTP_CODE::NO_REQUEST;
}
/*下面判断是否把长度为m_content_length
的数据读完了*/
http_conn::HTTP_CODE http_conn::parse_content(char *text)//其实post请求只会有用户名和密码的内容
{
    if (m_read_idx >= (m_content_length + m_checked_idx))//这样判断的是 是否用户名和密码已经读到缓冲区了
    {
        text[m_content_length] = '\0';
        //POST请求中最后为输入的用户名和密码
        m_string = text;
        return HTTP_CODE::GET_REQUEST;
    }
    return HTTP_CODE::NO_REQUEST;
}
http_conn::HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_status = LINE_STATUS::LINE_OK;
    HTTP_CODE ret = HTTP_CODE::NO_REQUEST;
    char *text = 0;
    //后面的parse_line是搞出一行出来，就是再一行的后面加'\0'，从缓冲区内
    while ((m_check_state == CHECK_STATE::CHECK_STATE_CONTENT && line_status == LINE_STATUS::LINE_OK) || ((line_status = parse_line()) == LINE_STATUS::LINE_OK))
    {
        text = get_line();//return m_read_buf + m_start_line;
        //text是一行内容
        m_start_line = m_checked_idx;
        LOG_INFO("%s", text);
        switch (m_check_state)
        {
        case CHECK_STATE::CHECK_STATE_REQUESTLINE:
        {
            ret = parse_request_line(text);//对第一行进行分析 并生成资源文件路径位置
            //读完第一行后  设置m_check_state为CHECK_STATE::CHECK_STATE_HEADER
            if (ret == HTTP_CODE::BAD_REQUEST)
                return HTTP_CODE::BAD_REQUEST;
            break;
        }
        case CHECK_STATE::CHECK_STATE_HEADER:
        {
            ret = parse_headers(text);
            //把请求头处理完后，才会设置m_check_state为CHECK_STATE::CHECK_STATE_CONTENT 没读完就一直一行一行的读
            if (ret == HTTP_CODE::BAD_REQUEST)
                return HTTP_CODE::BAD_REQUEST;
            else if (ret == HTTP_CODE::GET_REQUEST)//标识没有内容了，请求告一段落
            {
                return do_request();//处理要请求的文件，并把文件加载到内存中  然后返回文件请求 http_code
            }
            break;
        }
        case CHECK_STATE::CHECK_STATE_CONTENT:
        {
            ret = parse_content(text);//把用户名和密码存入了m_string
            if (ret == HTTP_CODE::GET_REQUEST)
                return do_request();
            line_status = LINE_STATUS::LINE_OPEN;
            break;
        }
        default:
            return HTTP_CODE::INTERNAL_ERROR;
        }
    }
    return HTTP_CODE::NO_REQUEST;
}
/*把请求的文件路径设置好，打开并映射到内存中*/
http_conn::HTTP_CODE http_conn::do_request()
{
    // strcpy(m_real_file,doc_root);
    m_real_file = doc_root;
    int len = doc_root.size();
    int index = m_url.find('/');
    // const char *p = strrchr(m_url,'/');
    /*cgi == 1 表示是post请求 post请求有两个 一个是注册一个是登录*/

    /*这里可以用策略模式+工厂模式，封装不同算法，*/
    if(cgi == 1 && m_url[index+1]=='2' || m_url[index+1] == '3')
    {
        int gap = m_string.find('&');
        std::string name = m_string.substr(5,gap-5);
        std::string password = m_string.substr(gap+10);
        /*注册界面*/
        if(m_url[index+1] == '3')
        {
            /*拼接mysql插入语句*/
            std::string sql_insert = "INSERT INTO user(username, passwd) VALUES('";
            sql_insert += name;
            sql_insert += "', '";
            sql_insert += password;
            sql_insert +="')";

            /*下面的不会注册错误，只要名字不重复就注册成功
             只会出现重复注册的错误*/
            if(users.find(name) == users.end())
            {
                int res;
                {
                    std::mutex lock;
                    std::lock_guard<std::mutex> lk(lock);
                    res = mysql_query(mysql.get(),sql_insert.c_str());
                    users.insert(std::pair<std::string,std::string>(name,password));
                }
                if(res != 0)
                    m_url = "/log.html";
                else
                    m_url = "/registerError.html";
            }
            m_url = "/registerError.html";
        }
        else if(m_url[index+1] == '2')
        {
            if(users.find(name) != users.end() && users[name] == password)
            /*这里不是+=*/
                m_url = "/welcome.html";
            else
                m_url = "/logError.html";
        }
    }
    /*注册界面*/
    if(m_url[index+1] == '0')
    {
        m_real_file += "/register.html";
    }
    else if (m_url[index+1] == '1')//请求登录的界面
    {
        m_real_file += "/log.html";
    }
    else if (m_url[index+1] == '5')
    {
        m_real_file += "/picture.html";
    }
    else if (m_url[index+1] == '6')
    {
        m_real_file += "/video.html";
    }
    else if (m_url[index+1] == '7')
    {
        m_real_file += "/fans.html";
    }
    else
        m_real_file += m_url;
    
    if(stat(m_real_file.c_str(),&m_file_stat) < 0)
        return HTTP_CODE::NO_REQUEST;
    if(!(m_file_stat.st_mode & S_IROTH))
        return HTTP_CODE::FORBIDDEN_REQUEST;
    if(S_ISDIR(m_file_stat.st_mode))
        return HTTP_CODE::BAD_REQUEST;
    int fd = open(m_real_file.c_str(),O_RDONLY);
    m_file_address = (char *)mmap(0,m_file_stat.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    close(fd);
    return HTTP_CODE::FILE_REQUEST;
}
void http_conn::unmap()
{
    if(m_file_address)
    {
        munmap(m_file_address,m_file_stat.st_size);
        m_file_address = 0;
    }
}
bool http_conn::write()
{
    int temp = 0;

    if(bytes_to_send == 0)
    {
        Utils_fd::modfd(m_epollfd,m_sockfd,EPOLLIN,m_TRIGMode);
        init();
        return true;
    }
    while(true)
    {
        temp = writev(m_sockfd,m_iv,m_iv_count);
        if(temp < 0)
        {
            if(errno == EAGAIN)
            {
                Utils_fd::modfd(m_epollfd,m_sockfd,EPOLLOUT,m_TRIGMode);
                return true;
            }
            unmap();
            return false;
        }

        bytes_have_send += temp;
        bytes_to_send -= temp;
        if(bytes_have_send >= m_iv[0].iov_len)
        {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        }
        else
        {
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }
        /*文件发完了，映射的内存可以释放了*/
        if (bytes_to_send <= 0)
        {
            unmap();
            Utils_fd::modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);

            if (m_linger)
            {
                init();
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}
bool http_conn::add_response(const char *format, ...)
{
    if(m_write_idx >= WRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list,format);

    int len = vsnprintf(m_write_buf+m_write_idx,WRITE_BUFFER_SIZE-1-m_write_idx,format,arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
    {
        va_end(arg_list);
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);//结束对可变参数的操作

    LOG_INFO("request:%s", m_write_buf);

    return true;
}
/*向用户写缓冲区写入一些内容*/
bool http_conn::add_status_line(int status, const char *title)//添加状态行
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);//向写缓冲区写东西
}
bool http_conn::add_headers(int content_len)
{
    return add_content_length(content_len) && add_linger() &&
           add_blank_line();
}
bool http_conn::add_content_length(int content_len)
{
    return add_response("Content-Length:%d\r\n", content_len);
}
bool http_conn::add_content_type()
{
    return add_response("Content-Type:%s\r\n", "text/html");
}
bool http_conn::add_linger()
{
    return add_response("Connection:%s\r\n", (m_linger == true) ? "keep-alive" : "close");
}
bool http_conn::add_blank_line()
{
    return add_response("%s", "\r\n");
}
bool http_conn::add_content(const char *content)
{
    return add_response("%s", content);
}

bool http_conn::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
    case HTTP_CODE::INTERNAL_ERROR:
    {
        add_status_line(500, error_500_title);//文件开头定义了这些内容  这句是把状态行写入写缓冲区
        add_headers(strlen(error_500_form));//基本上就是相应请求的返回头
        if (!add_content(error_500_form))
            return false;
        break;
    }
    case HTTP_CODE::BAD_REQUEST:
    {
        add_status_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
            return false;
        break;
    }
    case HTTP_CODE::FORBIDDEN_REQUEST:
    {
        add_status_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form))
            return false;
        break;
    }
    case HTTP_CODE::FILE_REQUEST:
    {
        add_status_line(200, ok_200_title);
        if (m_file_stat.st_size != 0)
        {
            add_headers(m_file_stat.st_size);
            m_iv[0].iov_base = m_write_buf;//指向写缓冲区
            m_iv[0].iov_len = m_write_idx;//写缓冲区的内容大小
            m_iv[1].iov_base = m_file_address;//指向文件
            m_iv[1].iov_len = m_file_stat.st_size;//文件大小
            m_iv_count = 2;
            bytes_to_send = m_write_idx + m_file_stat.st_size;
            return true;
        }
        else
        {
            const char *ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            if (!add_content(ok_string))
                return false;
        }
    }
    default:
        return false;
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;//这个是没有请求文件资源，只有自己写的缓冲区内容
    return true;
}
/*先会有读时间
    先一下子把数据读到用户读缓冲区
    一行一行的分析读到的数据
    根据分析的数据设置一些连接的状态（文件位置等）
        连接类的一些属性
        找到请求的文件并打开映射到内存
    然后根据读到的信息组织发送的包
    组织完后设置文件描述符可写*/
void http_conn::process()
{
    HTTP_CODE read_ret = process_read();
    //process_read 这个函数已经把用户缓冲区的内容读完了，并且进行了解析，然后把请求的文件加载到了内存
    if (read_ret == HTTP_CODE::NO_REQUEST)//HTTP_CODE::NO_REQUEST 应该就是内容没有读完
    {
        Utils_fd::modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);//继续监听
        return;
    }
    bool write_ret = process_write(read_ret);//根据上面的读操作的返回值设置写缓冲区的内容（就是分析读的结果，然后设置写缓冲区）
    if (!write_ret)
    {
        close_conn();
    }
    Utils_fd::modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode);
    //EPOLLOUT的触发条件之一，刚注册的就可以直接触发
    //写缓冲区从满到不满时直接触发，
    //客户端套接字刚连接可触发
    //上述时重新注册可触发
}
