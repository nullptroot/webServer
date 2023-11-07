#include "timeslice.h"
#include <assert.h>

time_wheel::~time_wheel()
{
    for(int i = 0; i < N; ++i)
    {
        tw_timer *tmp = slots[i];
        while(tmp != nullptr)
        {
            slots[i] = tmp->next;
            delete tmp;
            tmp = slots[i];
        }
    }
}
std::pair<int,int> time_wheel::compute_rotation_timeslot(int timeout)
{
    int ticks = 0;
    if(timeout < SI)
        ticks = 1;
    else
        ticks = timeout / SI;
    int rotation = ticks / N;
    int ts = (cur_slot + (ticks % N)) % N;
    return {rotation,ts};
}
tw_timer * time_wheel::add_timer(int timeout)
{
    
    if(timeout < 0)
        assert("error timeout must bigger zero\n");
    std::pair<int,int> tmp = compute_rotation_timeslot(timeout);

    tw_timer *timer = new tw_timer(tmp.first,tmp.second);
    /*这里原来是下面函数的内容，可以直接调用函数这里*/
    add_timer(timer);
    return timer;
}

void time_wheel::add_timer(tw_timer *timer)
{
    int ts = timer->time_slot;
    /*这里添加没有按照顺序来添加的，只是单纯的双链表
    可以按照rotation进行排序来实现优化*/
    if(slots[ts] == nullptr)
    {
        printf("add timer,rotation is %d,ts is %d,cur_slot is %d\n",timer->rotation,ts,cur_slot);
        slots[ts] = timer;
    }
    else
    {
        tw_timer *temp = slots[ts];
        if(temp->rotation > timer->rotation)
        {
            timer->next = slots[ts];
            slots[ts]->prev = timer;
            slots[ts] = timer;
        }
        else
        {
            while(temp->rotation < timer->rotation)
                temp = temp->next;
            timer->prev = temp->prev;
            temp->prev->next = timer;
            temp->prev = timer;
            timer->next = temp;
        }
    }
}
/*只是 从wheel中删除节点，但是不释放内存呢
增加此方法为了后续的调整操作*/
void time_wheel::del_timer_from_wheel(tw_timer *timer)
{
    int ts = timer->time_slot;
    if(timer == slots[ts])
    {
        slots[ts] = slots[ts]->next;
        if(slots[ts] != nullptr)
            slots[ts]->prev = nullptr;
    }
    else
    {
        timer->prev->next = timer->next;
        if(timer->next != nullptr)
            timer->next->prev = timer->prev;
    }
}
void time_wheel::del_timer(tw_timer *timer)
{
    if(timer == nullptr)
        return;
    del_timer_from_wheel(timer);
    delete timer;
}
/*调整定时器，先从时间轮中取出来（删除），然后更改
其状态信息，再加回去*/
void time_wheel::adjust_timer(tw_timer *timer,int timeout)
{
    if(timer == nullptr || timeout < 0)
        return;
    del_timer_from_wheel(timer);
    std::pair<int,int> tmp = compute_rotation_timeslot(timeout);
    timer->rotation = tmp.first;
    timer->time_slot = tmp.second;
    add_timer(timer);
}
void time_wheel::tick()
{
    tw_timer *tmp = slots[cur_slot];
    printf("current slot is %d\n",cur_slot);
    /*这里每次都需要遍历cur_slot链表的所有元素*/
    while(tmp->rotation == 0)
        tmp = tmp->next;
    slots[cur_slot] = tmp;
    /*tmp前面的都需要删除*/
    if(tmp->prev != nullptr)
    {
        tmp = tmp->prev;
        tw_timer *pre = tmp->prev;
        while(tmp != nullptr)
        {
            delete tmp;
            tmp = pre;
            if(pre != nullptr)
                pre = pre->prev;
        }
    }
    /*后面的rotation都需要减一*/
    slots[cur_slot]->prev = nullptr;
    tmp = slots[cur_slot];
    while(tmp != nullptr)
    {
        --tmp->rotation;
        tmp = tmp->next;
    }
    /*槽增加*/
    cur_slot = (++cur_slot) % N;
}