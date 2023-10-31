#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <list>
#include <exception>
#include <pthread.h>
#include "../locker/locker.h"
#include "../CGImysql/sql_connection_pool.h"
template <typename T>
class threadpool
{
    public:
        threadpool(int actor_model,connection_pool *connPool,int thread_number = 8,int max_request = 10000);
        ~threadpool();
        bool append(T *request,int state);
        bool append_p(T *request);
    private:
        static void *worker(void *arg);
        void run();
    private:
        int m_thread_number;
        int m_max_request;
        pthread_t *m_threads;
        std::list<T *> m_workqueue;
        locker m_queuelocker;
        sem m_queuestat;
        /*connection pool*/
        connection_pool *m_connPool;
        int m_actor_model;
};
template<typename T>
threadpool<T>::threadpool(int actor_model,connection_pool *connPool,int thread_number,int max_request):m_actor_model(actor_model),m_thread_number(thread_number), m_max_request(max_request), m_threads(NULL),m_connPool(connPool)
{
    if(thread_number <= 0 || max_request <= 0)
        throw std::exception();
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads)
        throw std::exception();
    for(int i = 0; i < thread_number; ++i)
    {
        if(pthread_create(m_threads+i,nullptr,worker,this))
        {
            delete [] m_threads;
            throw std::exception();
        }
        if(pthread_detach(m_threads[i]))
        {
            delete [] m_threads;
            throw std::exception();
        }
    }
}
template <typename T>
threadpool<T>::~threadpool()
{
    delete [] m_threads;
}
/*代码重复了  之后看看能不能代码复用*/
template <typename T>
bool threadpool<T>::append(T *reuqest,int state)
{
    m_queuelocker.lock();
    if(m_workqueue.size() >= m_max_request)
    {
        m_queuelocker.unlock();
        return false;
    }
    reuqest->m_state = state;
    m_workqueue.push_back(reuqest);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}
template <typename T>
bool threadpool<T>::append_p(T *request)
{
    m_queuelocker.lock();
    if(m_workqueue.size() >= m_max_request)
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}
template <typename T>
void * threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}
/*感觉这个run拉高了请求类和线程类的耦合，导致这个线程类只能处理请求类了*/
template <typename T>
void threadpool<T>::run()
{
    while(true)
    {
        m_queuestat.wait();
        m_queuelocker.lock();
        if(m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if(!request)
            continue;
        if(m_actor_model == 1)
        {
            if(request->m_state == 0)
            {
                if(request->read_once())
                {
                    request->improv = 1;
                    /*connection 操作*/
                    /*从连接池中取一个赋给request->mysql 相当于给
                    http_conn一个mysql连接*/
                    connectionRAII mysqlcon(&request->mysql, m_connPool);
                    request->process();
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
            else
            {
                if(request->write())
                    request->improv = 1;
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
        else
        {
            /*connection mysql*/
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }   
    }
}
#endif