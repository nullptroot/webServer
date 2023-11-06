#ifndef THREADPOOL11_H
#define THREADPOOL11_H
#include <list>
#include <vector>
#include <exception>
#include <thread>
#include <mutex>
#include <semaphore>
#include <memory>
#include "../log/block_queue11.h"
#include "../CGImysql/sql_connection_pool11.h"
#include "../http/http_conn11.h"
// #include <condition_variable>
#include "../locker/locker.h"
template <typename T>
class threadpool
{
    public:
        threadpool(int actor_model,int thread_number = 8,int max_request = 10000);
        ~threadpool() = default;
        bool append(T *request,int state);
        bool append_p(T *request);
        void stop(){stop_threadpoll = true}
    private:
        void worker();
        void run();
        void (*process)(T *request);
    private:
        int m_thread_number;
        int m_max_request;
        bool stop_threadpoll;/*线程池停止*/
        std::vector<std::thread> m_threads;
        /*这里直接使用阻塞队列了*/
        block_queue<T *> m_workqueue;
        // std::list<T *> m_workqueue;/*这里的list 可以直接实现一个线程安全list 封装成一个类*/
        // std::mutex m_mutex;
        /*刚刚考虑了一下，发现这里不能使用条件变量来代替信号量
        因为这里有一个数量的关系，条件变量不管list中有多少元素
        只会通知一次，而且wait在没有伪唤醒的情况下，只有等待唤
        醒，因此不管list中有多少元素，只会通知一次，这样的话就
        会出现list中有元素，但是线程还在wait，信号量就不一样了
        信号量会维持一个计数，当list中有元素的时候不会阻塞，直
        接向下执行*/
        // std::condition_variable m_queuestat;
        // sem m_queuestat;
        // /*connection pool*/


        /*这里的连接池不好，为什么要在线程池里
        给连接分配mysql连接呢，奇奇怪怪*/
        // connection_pool *m_connPool;
        int m_actor_model;
};
/*这里的函数可以换成其他的 赋给线程池*/
void prces(http_conn *request,int model)
{
    if(model == 1)
    {
        if(request->m_state == 0)
        {
            if(request->read_once())
            {
                request->improv = 1;
                /*connection 操作*/
                // request->mysql = m_connPool->GetConnection();
                // connectionRAII mysqlcon(&request->mysql, m_connPool);
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
        // request->mysql = m_connPool->GetConnection();
        // connectionRAII mysqlcon(&request->mysql, m_connPool);
        request->process();
    }   
}
template <typename T>
threadpool<T>::threadpool(int actor_model,int thread_number,int max_request):
                            m_thread_number(thread_number), m_max_request(max_request), 
                            m_actor_model(actor_model)
{
    process = prces;
    stop_threadpoll = false;
    if(thread_number <= 0 || max_request <= 0)
        throw std::exception();
    for(int i = 0; i < thread_number; ++i)
    {
        m_threads.emplace_back(&threadpool::worker,this);
        std::cout<<"create "<<i<<"th threads"<<std::endl;
        m_threads[i].detach();
    }
}
template <typename T>
bool threadpool<T>::append(T *reuqest,int state)
{
        // std::unique_lock<std::mutex> lk(m_mutex);
    if(m_workqueue.size() >= m_max_request || stop_threadpoll)
        return false;
    reuqest->m_state = state;
    m_workqueue.push(reuqest);
    // m_queuestat.notify_all();
    // m_queuestat.post();
    return true;
}
template <typename T>
bool threadpool<T>::append_p(T *request)
{
    // {
        // std::unique_lock<std::mutex> lk(m_mutex);
    if(m_workqueue.size() >= m_max_request || stop_threadpoll)
        return false;
    m_workqueue.push(request);
    // }
    // m_queuestat.notify_all();
    // m_queuestat.post();
    return true;
}
template <typename T>
void  threadpool<T>::worker()
{
    this->run();
}
template <typename T>
void threadpool<T>::run()
{
    while(!stop_threadpoll)
    {
        T *request = nullptr;
        // m_queuestat.wait();
        // {
            // m_queuelocker.lock();
            // std::lock_guard<std::mutex> lk(m_mutex);
            // m_queuestat.wait(lk,[this]{return !m_workqueue.empty()})
            // request = m_workqueue.front();
        m_workqueue.pop(request);
        // }
        if(!request)
            continue;
        process(request,m_actor_model);
    }
}
#endif