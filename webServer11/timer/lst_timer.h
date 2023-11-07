#ifndef LST_TIMER_H
#define LST_TIMER_H

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

#include <time.h>
#include "client_data.h"
// #include "../log/log.h"



class util_timer
{
    public:
        time_t expire;
        void (*cb_func)(client_data<util_timer> *);
        client_data<util_timer> *user_data;
        util_timer *prev;
        util_timer *next;
    public:
        util_timer() : prev(NULL), next(NULL) {}
};

class sort_timer_lst
{
    public:
        sort_timer_lst();
        ~sort_timer_lst();

        void add_timer(util_timer *timer);
        void adjust_timer(util_timer *timer);
        void del_timer(util_timer *timer);
        void tick();
    private:
        void add_timer(util_timer *timer,util_timer *lst_head);
        util_timer *head;
        util_timer *tail;
};

/*这里的回调不用在这里实现，在构造定时器的时候传入就行了*/
// void cb_func(client_data *user_data);
#endif