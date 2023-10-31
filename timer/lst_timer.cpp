#include "lst_timer.h"
#include "../http/http_conn.h"

sort_timer_lst::sort_timer_lst()
{
    head = nullptr;
    tail = nullptr;
}
sort_timer_lst::~sort_timer_lst()
{
    util_timer *tmp = head;
    while(tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer)
{
    if(!timer)
        return;
    if(!head)
    {
        head = tail = timer;
        return;
    }
    if(timer->expire < head->expire)
    {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    /*加到head后面*/
    add_timer(timer,head);
}

void sort_timer_lst::adjust_timer(util_timer *timer)
{
    if(!timer)
        return;
    util_timer *tmp = timer->next;
    if(!tmp || (timer->expire < tmp->expire))
        return;
    if(timer == head)
    {
        /*timer == head时，换head，然后把timer插入到head后*/
        head = head->next;
        head->prev = nullptr;
        timer->next = nullptr;
        add_timer(timer,head);
    }
    else
    {
        /*其他情况就是，删除timer，使其加到timer->next后
        这里删除timer是，从链表中删除*/
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer,timer->next);
    }
}
void sort_timer_lst::del_timer(util_timer *timer)
{
    if(!timer)
        return;
    /*仅有一个定时器*/
    if((timer == head) && (timer == tail))
    {
        delete timer;
        head = nullptr;
        tail = nullptr;
        return;
    }
    if(timer == head)
    {
        head = head->next;
        tail->next = nullptr;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

void sort_timer_lst::tick()
{
    if(!head)
        return;
    time_t cur = time(nullptr);
    util_timer *tmp = head;
    while(tmp != nullptr)
    {
        /*过期就删除 这是有序链表嗷*/
        if(cur < tmp->expire)
            break;
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if(head != nullptr)
            head->prev = nullptr;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer,util_timer *lst_head)
{
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;
    while(tmp != nullptr)
    {
        if(timer->expire < tmp->expire)
        {
            prev->next = timer;
            timer->prev = prev;
            timer->next = tmp;
            tmp->prev = timer;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    /*说明是因为tmp不满足while结束的，而不是break*/
    if(tmp == nullptr)
    {
        prev->next = timer;
        timer->prev = prev;
        timer->next = nullptr;
        tail = timer;
    }
}
void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

void Utils::addfd(int epollfd,int fd,bool one_shot,int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;
    if(1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;
    if(one_shot == true)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

void Utils::sig_handler(int sig)
{
    int save_error = errno;
    int msg = sig;
    send(u_pipefd[1],(char *)&msg,1,0);
    errno = save_error;
}

void Utils::addsig(int sig,void(*handler)(int),bool restart)
{
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = handler;
    if(restart == true)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,nullptr) != -1);
}

void Utils::timer_handler()
{
    m_timer_lst.tick();
    /*每次处理完，需要再次设置timeslot*/
    alarm(m_TIMESLOT);
}
void Utils::show_error(int connfd,const char *info)
{
    send(connfd,info,strlen(info),0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;

void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd,EPOLL_CTL_DEL,user_data->sockfd,0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}