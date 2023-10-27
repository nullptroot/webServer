#ifndef _CONNECTION_POLL_H
#define _CONNECTION_POLL_H

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../locker/locker.h"
#include "../log/log.h"

class connection_pool
{
    public:
        MYSQL *GetConnection();
        bool ReleaseConnection(MYSQL *conn);
        int GetFreeConn();
        void DestroyPool();

        static connection_pool *GetInstance();

        void init(std::string url,std::string User,std::string PassWord,std::string DataBaseName,
                    int Port, int MaxConn,int close_log);
    private:
        connection_pool();
        ~connection_pool();

        int m_MaxConn;
        int m_CurConn;
        int m_FreeConn;
        locker lock;
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

class connectionRAII
{
    private:
        MYSQL *connRAII;
        connection_pool *poolRAII;
    public:
        connectionRAII(MYSQL **con,connection_pool *connPool);
        ~connectionRAII();
};
#endif
