#include "sql_connection_pool11.h"
#include <pthread.h>

connection_pool::connection_pool():reserve(0)
{
    m_CurConn = 0;
    m_FreeConn = 0;
}
/*单例模式*/
connection_pool *connection_pool::GetInstance()
{
    static connection_pool connPool;
    return &connPool;
}
/*这里连接  如果连接比较多的话
就可以使用异步或者携程进行数据库连接*/
bool connection_pool::mysqlConnect(MYSQL **con)
{
    *con = mysql_init(*con);
    if(*con == nullptr)
    {
        LOG_ERROR("MySQL Error,%s %d",__FILE__,__LINE__);
        return false;
    }
    *con = mysql_real_connect(*con,m_url.c_str(),m_User.c_str(),m_PassWord.c_str(),
                                m_DatabaseName.c_str(),m_Port,NULL,0);
    if(*con == nullptr)
    {
        LOG_ERROR("MySQL Error, %s %d",__FILE__,__LINE__);
        return false;
    }
    return true;
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
        if(mysqlConnect(&con) == false)
            exit(1);
        connList.push_back(con);
        ++m_FreeConn;
    }
    reserve.release(m_FreeConn);
}
std::unique_ptr<MYSQL,connection_pool::deleteFunc> connection_pool::GetConnection()
{
    // MYSQL *con = nullptr;
    if(connList.size() == 0)
        return std::unique_ptr<MYSQL,deleteFunc>(nullptr,deleteFunc(this));
    MYSQL *con;
    reserve.acquire();
    {
        std::lock_guard<std::mutex> lk(lock);
        con = connList.front();
        connList.pop_front();
        --m_FreeConn;
        ++m_CurConn;
    }
    return std::unique_ptr<MYSQL,connection_pool::deleteFunc>(con,deleteFunc(this));
}

void connection_pool::ReleaseConnection(MYSQL *con)
{
    if(con == nullptr)
        return;
    {
        std::lock_guard<std::mutex> lk(lock);
        connList.push_back(con);
        ++m_FreeConn;
        --m_CurConn;
    }
    reserve.release();
}

void connection_pool::DestroyPool()
{

	std::lock_guard<std::mutex> lk(lock);
	if (connList.size() > 0)
	{
        for(auto i:connList)
            mysql_close(i);
		m_CurConn = 0;
		m_FreeConn = 0;
		connList.clear();
	}
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