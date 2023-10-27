#include "sql_connection_pool.h"
#include <pthread.h>

connection_pool::connection_pool()
{
    m_CurConn = 0;
    m_FreeConn = 0;
}

connection_pool *connection_pool::GetInstance()
{
    static connection_pool connPool;
    return &connPool;
}
void connection_pool::init(std::string url,std::string User,std::string PassWord,std::string DataBaseName,
                    int Port, int MaxConn,int close_log)
{
    m_url = url;
    m_Port = Port;
    m_User = User;
    m_PassWord = PassWord;
    m_DatabaseName = DataBaseName;
    m_close_log = close_log;
    /*源代码好像没有这句最大连接数的赋值操作
    
    在下面出现了*/
    m_MaxConn = MaxConn;

    for(int i = 0; i < MaxConn; ++i)
    {
        MYSQL *con = nullptr;
        con = mysql_init(con);
        if(con == nullptr)
        {
            LOG_ERROR("MySQL Error,%s %d",__FILE__,__LINE__);
            exit(1);
        }
        /*写的时候有个bug，就是当mysql服务端不允许本地root连接时
        当执行 mysql_real_connect时，会被拒绝，但是mysql_error是
        检测不出错误的，此时con会是一个nullptr，而服务端
        好像也会建立连接，但是直接就是timewait状态，也就是
        服务器会主动断开连接的  但是在客户端没有错误码设置（perror打印
        success）*/
        con = mysql_real_connect(con,m_url.c_str(),m_User.c_str(),m_PassWord.c_str(),
                                    m_DatabaseName.c_str(),Port,NULL,0);
        if(con == nullptr)
        {
            LOG_ERROR("MySQL Error, %s %d",__FILE__,__LINE__);
            exit(1);
        }

        connList.push_back(con);
        ++m_FreeConn;
    }
    reserve = sem(m_FreeConn);
}
MYSQL *connection_pool::GetConnection()
{
    MYSQL *con = nullptr;
    if(connList.size() == 0)
        return con;
    reserve.wait();
    lock.lock();

    con = connList.front();
    connList.pop_front();

    --m_FreeConn;
    ++m_CurConn;

    lock.unlock();
    return con;
}

bool connection_pool::ReleaseConnection(MYSQL *con)
{
    if(con == nullptr)
        return false;
    lock.lock();

    connList.push_back(con);
    ++m_FreeConn;
    --m_CurConn;

    lock.unlock();

    reserve.post();
    return true;
}

void connection_pool::DestroyPool()
{
    lock.lock();
    if(connList.size() > 0)
    {
        std::list<MYSQL *>::iterator it;
        for(it = connList.begin(); it != connList.end(); ++it)
        {
            MYSQL *con = *it;
            mysql_close(con);
        }
        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear();
    }
    lock.unlock();
}
/*这在多线程下 不一定是准的*/
int connection_pool::GetFreeConn()
{
    return this->m_FreeConn;
}
connection_pool::~connection_pool()
{
    DestroyPool();
}
/*感觉有点臃肿，每次都要把连接池放进去赋值*/
connectionRAII::connectionRAII(MYSQL **SQL,connection_pool *connPool)
{
    *SQL = connPool->GetConnection();
    connRAII = *SQL;
    poolRAII = connPool;
}
connectionRAII::~connectionRAII()
{
    poolRAII->ReleaseConnection(connRAII);
}