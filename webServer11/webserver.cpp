#include "webserver.h"
#include <arpa/inet.h>
#include "timer/lst_timer.h"
#include "timer/Utils.h"
#include <iostream>

WebServer::WebServer()
{
    // users = new http_conn[MAX_FD];
    users.resize(MAX_FD);

    char server_path[200];
    getcwd(server_path,200);

    // char root[6] = "/root";
    std::string root = "/root";
    // m_root = (char *)malloc(strlen(server_path)+strlen(root)+1);
    m_root = server_path;
    m_root += root;
    // strcpy(m_root,server_path);
    // strcat(m_root,root);

    // users_timer = new client_data<util_timer>[MAX_FD];
    users_timer.resize(MAX_FD);
}
WebServer::~WebServer()
{
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[0]);
    close(m_pipefd[1]);
    // delete [] users;
    // delete [] users_timer;
    // delete m_pool;
}

void WebServer::init(int port,std::string user,std::string passWord,std::string databaseName,
                    int log_write,int opt_linger,int trigmode,int sql_num,
                    int thread_num,int close_log,int actor_model)
{
    m_port = port;
    m_user = user;
    m_passWord = passWord;
    m_databaseName = databaseName;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_log_write = log_write;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = trigmode;
    m_close_log = close_log;
    m_actormodel = actor_model;
}
void WebServer::trig_mode()
{
    if(m_TRIGMode == 0)
    {
        m_LISTENTigmode = 0;
        m_CONNTrigmode = 1;
    }
    else if(m_TRIGMode == 1)
    {
        m_LISTENTigmode = 0;
        m_CONNTrigmode = 1;
    }
    else if(m_TRIGMode == 2)
    {
        m_LISTENTigmode = 1;
        m_CONNTrigmode = 0;
    }
    else if(m_TRIGMode == 3)
    {
        m_LISTENTigmode = 1;
        m_CONNTrigmode = 1;
    }
}

void WebServer::log_write()
{
    if(m_close_log == 0)
    {
        if(m_log_write == 1)
            Log::get_instance()->init("./ServerLog",m_close_log,2000,800000,800);
        else
            Log::get_instance()->init("./ServerLog",m_close_log,2000,800000,0);
    }
}

void WebServer::sql_pool()
{
    m_connPool = connection_pool::GetInstance();

    m_connPool->init("localhost",m_user,m_passWord,m_databaseName,3306,m_sql_num,m_close_log);

    /*这里好奇怪，刚开始初始化就这样搞吗，而且users是个指针指向一块多个
    http_conn的内存
    整个代码就这里使用了一次
    奇奇怪怪  而且这里只是把用户名和密码存到全局变量里*/
    http_conn::initmysql_result(m_connPool);
    http_conn::initMethodMap();
}

void WebServer::thread_pool()
{
    m_pool.reset(new threadpool<http_conn>(m_actormodel,m_thread_num));
}
void WebServer::eventListen()
{
    m_listenfd = socket(PF_INET,SOCK_STREAM,0);
    assert(m_listenfd >= 0);

    if(m_OPT_LINGER == 0)
    {
        struct linger tmp = {0,1};
        setsockopt(m_listenfd,SOL_SOCKET,SO_LINGER,&tmp,sizeof(tmp));
    }
    else if(m_OPT_LINGER == 1)
    {
        struct linger tmp = {1,1};
        setsockopt(m_listenfd,SOL_SOCKET,SO_LINGER,&tmp,sizeof(tmp));
    }

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int flag = 1;
    setsockopt(m_listenfd,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag));
    ret = bind(m_listenfd,(struct sockaddr *)&address,sizeof(address));
    assert(ret >= 0);
    ret = listen(m_listenfd,5);
    assert(ret >= 0);

    utils.init(TIMESLOT);

    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);
    Utils_fd::addfd(m_epollfd,m_listenfd,false,m_LISTENTigmode);
    http_conn::m_epollfd = m_epollfd;

    ret = socketpair(PF_UNIX,SOCK_STREAM,0,m_pipefd);
    assert(ret != -1);

    Utils_fd::setnonblocking(m_pipefd[1]);
    Utils_fd::addfd(m_epollfd,m_pipefd[0],false,0);

    Utils_fd::addsig(SIGPIPE,SIG_IGN);
    Utils_fd::addsig(SIGALRM,Utils_fd::sig_handler,false);
    Utils_fd::addsig(SIGTERM,Utils_fd::sig_handler,false);

    alarm(TIMESLOT);

    Utils_fd::u_pipefd = m_pipefd;
    Utils_fd::u_epollfd = m_epollfd;
}
/*回调函数*/
template <typename T>
void cb_func(client_data<T> *user_data)
{
    epoll_ctl(Utils_fd::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}

void WebServer::timer(int connfd,struct sockaddr_in client_address)
{
    users[connfd].init(connfd,client_address,m_root,m_CONNTrigmode,m_close_log,m_connPool);

    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;
    util_timer *timer = new util_timer;
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    /*这里就只能这样，因为不同的定时器 工作方式不一样*/
    timer->expire = cur + 3 * TIMESLOT;
    users_timer[connfd].timer = timer;
    utils.m_timer_lst.add_timer(timer);
}

void WebServer::adjust_timer(util_timer *timer)
{
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    utils.m_timer_lst.adjust_timer(timer);
    LOG_INFO("%s","adjust timer once");
}

void WebServer::deal_timer(util_timer *timer,int sockfd)
{
    
    timer->cb_func(&users_timer[sockfd]);
    if(timer)
        utils.m_timer_lst.del_timer(timer);
    LOG_INFO("close fd %d",users_timer[sockfd].sockfd);
}

bool WebServer::dealclientdata()
{
    struct sockaddr_in client_address;
    socklen_t client_addlengeh = sizeof(client_address);

    if(m_LISTENTigmode == 0)
    {
        int connfd = accept(m_listenfd,(struct sockaddr *)&client_address,&client_addlengeh);
        if(connfd < 0)
        {
            LOG_ERROR("%s:errno is %d","accept error",errno);
            return false;
        }
        if(http_conn::m_user_count >= MAX_FD)
        {
            utils.show_error(connfd,"Internal server busy");
            LOG_ERROR("%s","Internal server busy");
            return false;
        }
        timer(connfd,client_address);
    }
    else
    {
        while(true)
        {
            int connfd = accept(m_listenfd,(struct sockaddr *)&client_address,&client_addlengeh);
            if(connfd < 0)
            {
                LOG_ERROR("%s:errno is %d","accept error",errno);
                break;
            }
            if(http_conn::m_user_count >= MAX_FD)
            {
                utils.show_error(connfd,"Internal server busy");
                LOG_ERROR("%s","Internal server busy");
                break;
            }
            timer(connfd,client_address);
            return true;
        }
        /*这里对应上述的两个if条件句的break*/
        return false;
    }
    return true;
}

bool WebServer::dealwithsignal(bool &timeout,bool &stop_server)
{
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(m_pipefd[0],signals,sizeof(signals),0);
    if(ret == -1)
        return false;
    else if(ret == 0)
        return false;
    else
    {
        for(int i = 0; i < ret; ++i)
        {
            switch (signals[i])
            {
            case SIGALRM:
            {
                timeout = true;
                break;
            }
            case SIGTERM:
            {
                stop_server = true;
                break;
            }
            }
        }
    }
    return true;
}

void WebServer::dealwithread(int sockfd)
{
    util_timer *timer = users_timer[sockfd].timer;
    if(m_actormodel)
    {
        if(timer)
            adjust_timer(timer);
        m_pool->append(&users[sockfd],0);
        while(true)
        {
            if(users[sockfd].improv == 1)
            {
                if(users[sockfd].timer_flag == 1)
                {
                    deal_timer(timer,sockfd);
                    
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else
    {
        if(users[sockfd].read_once())
        {
            LOG_INFO("deal with the client(%s)",inet_ntoa(users[sockfd].get_address()->sin_addr));

            m_pool->append_p(&users[sockfd]);

            if(timer)
                adjust_timer(timer);
        }
        else
        {
            deal_timer(timer,sockfd);
        }
    }
}

void WebServer::dealwithwrite(int sockfd)
{
    util_timer *timer = users_timer[sockfd].timer;

    if(m_actormodel == 1)
    {
        if(timer)
            adjust_timer(timer);
        m_pool->append(&users[sockfd],1);
        while(true)
        {
            /*这里的判断应该时确定从内核读或者写的函数已经返回了*/
            if(users[sockfd].improv == 1)
            {
                if(users[sockfd].timer_flag == 1)
                {
                    deal_timer(timer,sockfd);
                    
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else
    {
        if(users[sockfd].write())
        {
            LOG_INFO("send data to client(%s)",inet_ntoa(users[sockfd].get_address()->sin_addr));
            if(timer)
                adjust_timer(timer);
        }
        else
        {
            deal_timer(timer,sockfd);
        }
    }
}

void WebServer::eventLoop()
{
    bool timeout = false;
    bool stop_server = false;

    while(!stop_server)
    {
        int number = epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);
        if(number < 0 && errno != EINTR)
        {
            LOG_ERROR("%s","epoll failure");
            break;
        }
        for(int i = 0; i < number; ++i)
        {
            int sockfd = events[i].data.fd;

            if(sockfd == m_listenfd)
            {
                bool flag = dealclientdata();
                if(flag == false)
                    continue;
            }
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer,sockfd);
            }
            else if((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                bool flag = dealwithsignal(timeout,stop_server);
                if(flag == false)
                    LOG_ERROR("%s","dealwithsignal failure")
            }
            else if(events[i].events & EPOLLIN)
            {
                dealwithread(sockfd);
            }
            else if(events[i].events & EPOLLOUT)
            {
                dealwithwrite(sockfd);
            }
        }
        if(timeout == 1)
        {
            utils.timer_handler();
            LOG_INFO("%s","timer tick");

            timeout = false;
        }
    }
}