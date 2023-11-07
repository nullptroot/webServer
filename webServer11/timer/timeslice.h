#include "client_data.h"
#ifndef TIME_WHEEL_TIMER_H
#define TIME_WHEEL_TIMER_H
#include <time.h>
#include <utility>
#include <netinet/in.h>
#include <stdio.h>
#define BUFFER_SIZE 64
/*这里的时间片轮询，已经做了修改
添加了新方法，之前提交的是有一些bug的，本地改了*/
class tw_timer
{
    public:
        int rotation;
        int time_slot;
        int timeout;
        void (*cb_func)(client_data<tw_timer> *);
        client_data<tw_timer> *user_data;
        tw_timer *next;
        tw_timer *prev;
    public:
        tw_timer(int rot,int ts):next(nullptr),prev(nullptr),rotation(rot),time_slot(ts){};
};

class time_wheel
{
    private:
        static const int N = 60;
        static const int SI = 1;
        tw_timer *slots[N];
        int cur_slot;
    public:
        time_wheel():cur_slot(0)
        {
            for(int i = 0; i < N; ++i)
                slots[i] = nullptr;
        }
        ~time_wheel();
        tw_timer *add_timer(int timeout);
        void add_timer(tw_timer *timer);
        void del_timer(tw_timer *timer);
        /*新加的调整*/
        void adjust_timer(tw_timer *timer,int timeout);
        void tick();
    private:
        /*从时间轮中取出来，并不释放内存，只是逻辑删除*/
        void del_timer_from_wheel(tw_timer *timer);
        /*应该有返回值优化  计算timeout的rotation和timeslot*/
        inline std::pair<int,int> compute_rotation_timeslot(int timeout);
};

#endif