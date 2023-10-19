/*这是多线程程序所需要的锁，条件变量和信号量*/
#ifndef LOCKER_H
#define LOCKER_H
#include <exception>
#include <pthread.h>
#include <semaphore.h>
/*这里的所有类 c++11中都有对应封装好的类
在用的时候可以都试试*/
/*信号量的封装*/
class sem
{
    private:
        sem_t m_sem;
    public:
        sem()
        {
            if(sem_init(&m_sem,0,0))
                throw std::exception();
        }
        sem(int num)
        {
            if(sem_init(&m_sem,0,num))
                throw std::exception();
        }
        ~sem()
        {
            sem_destroy(&m_sem);
        }
        bool wait()
        {
            return sem_wait(&m_sem);
        }
        bool post()
        {
            return sem_post(&m_sem);
        }
};

class locker
{
    private:
        pthread_mutex_t m_mutex;
    public:
        locker()
        {
            if(pthread_mutex_init(&m_mutex,nullptr))
                throw std::exception();
        }
        ~locker()
        {
            pthread_mutex_destroy(&m_mutex);
        }
        bool lock()
        {
            return pthread_mutex_lock(&m_mutex);
        }
        bool unlock()
        {
            return pthread_mutex_unlock(&m_mutex);
        }
        pthread_mutex_t *get()
        {
            return &m_mutex;
        }
};

class cond
{
    private:
        pthread_cond_t m_cond;
    public:
        cond()
        {
            if(pthread_cond_init(&m_cond,nullptr))
                throw std::exception();
        }
        ~cond()
        {
            pthread_cond_destroy(&m_cond);
        }
        bool wait(pthread_mutex_t *m_mutex)
        {
            return pthread_cond_wait(&m_cond,m_mutex);
        }
        bool timedwait(pthread_mutex_t *m_mutex,timespec t)
        {
            return pthread_cond_timedwait(&m_cond,m_mutex,&t);
        }
        bool signal()
        {
            return pthread_cond_signal(&m_cond);
        }
        bool broadcast()
        {
            return pthread_cond_broadcast(&m_cond);
        }
};
#endif