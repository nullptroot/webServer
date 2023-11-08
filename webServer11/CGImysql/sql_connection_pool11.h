#ifndef _CONNECTION_POLL_H
#define _CONNECTION_POLL_H
// #define __cplusplus 201907L
#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include <semaphore>
#include "../timer/Utils.h"
// #include "../../locker/locker.h"
#include "../log/log11.h"
#include <mutex>
#include <memory>

/*主要功能是给用户一个mysql连接
用户不使用的时候归还给连接池
那么共享指针的删除器就不能是
直接释放mysql连接

这里更改完 就不需要RAII类管理
资源了，直接返回一个右值unique_ptr
unique_ptr 结束就能直接回收
连接资源*/

using sem = std::binary_semaphore;

class connection_pool
{
    public:
        /*当返给用户的unique_ptr 生存期到了之后就会自动调用下面
        的函数回收mysql连接的资源*/
        class deleteFunc
        {
            private:
                connection_pool *pool;
            public:
                deleteFunc(connection_pool *poo):pool(poo){};
                void operator()(MYSQL *con)
                {
                    pool->ReleaseConnection(con);
                }
        };
    private:
        void ReleaseConnection(MYSQL *con);
        /*判断是否能连接mysql  并获得连接*/
        bool mysqlConnect(MYSQL **con);
        
    public:
        /*可以使用移动语义，因为反正链表也是要弹出的
        移动语义导致链表中的共享指针失效了 那好像可以使用unique_ptr*/
        std::unique_ptr<MYSQL,deleteFunc> GetConnection();
        /*采用右值引用，表示要回收资源了 因此就不必再保有资源了*/
        int GetFreeConn();
        void DestroyPool();
        /*静态变量是不需要内存管理的，程序结束后自动释放*/
        static connection_pool *GetInstance();
        
        void init(std::string url,std::string User,std::string PassWord,std::string DataBaseName,
                    int Port, int MaxConn,int close_log);
    private:

        connection_pool();
        ~connection_pool();

        int m_MaxConn;
        int m_CurConn;
        int m_FreeConn;
        std::mutex lock;
        std::list<MYSQL *> connList;
        sem reserve;
    public:
        std::string m_url;
        int m_Port; /*源代码 m_Port是string类型*/
        std::string m_User;
        std::string m_PassWord;
        std::string m_DatabaseName;
        int m_close_log;
};
#endif