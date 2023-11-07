#ifndef UTILS_H
#define UTILS_H

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

void Utils_fd::removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//将事件重置为EPOLLONESHOT
void Utils_fd::modfd(int epollfd, int fd, int ev, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int Utils_fd::setnonblocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}
void Utils_fd::addfd(int epollfd,int fd,bool one_shot,int TRIGMode)
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
void Utils_fd::sig_handler(int sig)
{
    int save_error = errno;
    int msg = sig;
    send(u_pipefd[1],(char *)&msg,1,0);
    errno = save_error;
}
void Utils_fd::addsig(int sig,void(*handler)(int),bool restart)
{
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = handler;
    if(restart == true)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,nullptr) != -1);
}
int *Utils_fd::u_pipefd = 0;
int Utils_fd::u_epollfd = 0;

#endif