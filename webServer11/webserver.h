#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "timer/Utils.h"
#include "timer/client_data.h"
#include "timer/lst_timer.h"

#include <string>
#include <vector>
#include <memory>

#include "./threadpool/threadpool_11.h"
#include "./http/http_conn11.h"
const int MAX_FD = 65535;
const int MAX_EVENT_NUMBER = 10000;
const int TIMESLOT = 5;

class WebServer
{
    public:
        int m_port;
        std::string m_root;
        int m_log_write;
        int m_close_log;
        int m_actormodel;

        int m_pipefd[2];
        int m_epollfd;

        // http_conn *users;
        std::vector<http_conn> users;
        connection_pool *m_connPool;
        std::string m_user;
        std::string m_passWord;
        std::string m_databaseName;
        int m_sql_num;

        std::unique_ptr<threadpool<http_conn>> m_pool;
        // threadpool<http_conn> *m_pool;

        int m_thread_num;

        epoll_event events[MAX_EVENT_NUMBER];

        int m_listenfd;
        int m_OPT_LINGER;

        int m_TRIGMode;
        int m_LISTENTigmode;
        int m_CONNTrigmode;

        // client_data<util_timer> *users_timer;
        std::vector<client_data<util_timer>> users_timer;
        Utils_Timer<sort_timer_lst> utils;
    public:
        WebServer();
        ~WebServer();

        void init(int port,std::string user,std::string passWord,std::string databaseName,
                    int log_write,int opt_linger,int trigmode,int sql_num,
                    int thread_num,int close_log,int actor_model);
        void thread_pool();
        void sql_pool();
        void log_write();
        void trig_mode();
        void eventListen();
        void eventLoop();
        void timer(int connfd,struct sockaddr_in client_address);
        void adjust_timer(util_timer *timer);
        void deal_timer(util_timer *timer,int sockfd);
        bool dealclientdata();
        bool dealwithsignal(bool &timeout,bool &stop_server);
        void dealwithread(int sockfd);
        void dealwithwrite(int sockfd);
};

#endif