#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H
#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../locker/locker.h"
/*这是个循环 带锁的线程安全队列*/
template<typename T>
class block_queue
{
    private:
        locker m_mutex;
        cond m_cond;
        T *m_array;
        int m_size;
        int m_max_size;
        int m_front;
        int m_back;
    public:
        block_queue(int max_size = 1000)
        {
            if(max_size <= 0)
                exit(-1);
            m_max_size = m_size;
            m_array = new T[max_size];
            m_size = 0;
            m_front = -1;
            m_back = -1;
        }
        void clear()
        {
            m_mutex.lock();
            m_size = 0;
            m_front = -1;
            m_back = -1;
            m_mutex.unlock();
        }
        ~block_queue()
        {
            m_mutex.lock();
            if(m_array != nullptr)
                delete [] m_array;
            m_mutex.unlock();
        }
        bool full()
        {
            m_mutex.lock();
            if(m_size >= m_max_size)
            {
                m_mutex.unlock();
                return true;
            }
            m_mutex.unlock();
            return false;
        }
        bool empty()
        {
            m_mutex.lock();
            if(m_size == 0)
            {
                m_mutex.unlock();
                return true;
            }
            m_mutex.unlock();
            return false;
        }
        bool front(T &value)
        {
            m_mutex.lock();
            if(0 == m_size)
            {
                m_mutex.unlock();
                return false;
            }
            value = m_array[m_front];
            m_mutex.unlock();
            return true;
        }
        bool back(T &value)
        {
            m_mutex.lock();
            if(0 == m_size)
            {
                m_mutex.unlock();
                return false;
            }
            value = m_array[m_back];
            m_mutex.unlock();
            return true;
        }
        int size()
        {
            int temp = 0;
            m_mutex.lock();
            temp = m_size;
            m_mutex.unlock();
            return temp;
        }
        int max_size()
        {
            int temp = 0;
            m_mutex.lock();
            temp = m_max_size;
            m_mutex.unlock();
            return temp;
        }
        bool push(const T &item)
        {
            m_mutex.lock();
            if(m_size >= m_max_size)
            {
                m_cond.broadcast();
                m_mutex.unlock();
                return false;
            }
            m_back = (m_back+1) % m_max_size;
            m_array[m_back] = item;
            m_size++;
            m_cond.broadcast();
            m_mutex.unlock();
            return true;
        }

        bool pop(T &item)
        {
            m_mutex.lock();
            while(m_size <= 0)
            {
                if(!m_cond.wait(m_mutex.get()))
                {
                    m_mutex.unlock();
                    return false;
                }
            }
            m_front = (m_front + 1) % m_max_size;
            item = m_array[m_front];
            m_size--;
            m_mutex.unlock();
            return true;
        }

        bool pop(T &item,int ms_timeout)
        {
            struct timespec t = {0,0};
            struct timeval now = {0,0};
            gettimeofday(&now,nullptr);
            m_mutex.lock();
            if(m_size <= 0)
            {
                t.tv_sec = now.tv_sec + ms_timeout / 1000;
                t.tv_nsec = (ms_timeout % 1000) * 1000;
                if(!m_cond.timedwait(m_mutex.get(),t))
                {
                    m_mutex.unlock();
                    return false;
                }
            }
            /*等这么长时间m_size还是没有大于0 就返回false*/
            if(m_size <= 0)
            {
                m_mutex.unlock();
                return false;
            }
            m_front = (m_front + 1) % m_max_size;
            item = m_array[m_front];
            m_size--;
            m_mutex.unlock();
            return true;
            
        }
};

#endif