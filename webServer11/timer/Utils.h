#ifndef UTILS_H
#define UTILS_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <semaphore.h>
#include <exception>

#include <time.h>
#include "client_data.h"

// class sem
// {
//     private:
//         sem_t m_sem;
//     public:
//         sem()
//         {
//             if(sem_init(&m_sem,0,0))
//                 throw std::exception();
//         }
//         sem(int num)
//         {
//             if(sem_init(&m_sem,0,num))
//                 throw std::exception();
//         }
//         ~sem()
//         {
//             sem_destroy(&m_sem);
//         }
//         bool wait()
//         {
//             return sem_wait(&m_sem);
//         }
//         bool post()
//         {
//             return sem_post(&m_sem);
//         }
// };

template <typename T>
class Utils_Timer
{
    public:
        T m_timer_lst;
        int m_TIMESLOT;
    public:
        Utils_Timer(){};
        ~Utils_Timer(){};

        void init(int timeslot){m_TIMESLOT = timeslot;};
        void timer_handler()
        {
            m_timer_lst.tick();
            /*每次处理完，需要再次设置timeslot*/
            alarm(m_TIMESLOT);
        }
        void show_error(int connfd,const char *info)
        {
            send(connfd,info,strlen(info),0);
            close(connfd);
        }
};

class Utils_fd
{
    public:
        static int *u_pipefd;
        static int u_epollfd;
    public:
        Utils_fd(){};
        ~Utils_fd(){};

        static int setnonblocking(int fd);

        static void addfd(int epollfd,int df,bool one_shot,int TRIGMode);

        static void addsig(int sig,void(*handler)(int),bool restart = true);

        static void sig_handler(int sig);

        static void modfd(int epollfd, int fd, int ev, int TRIGMode);

        static void removefd(int epollfd, int fd);

};


#endif