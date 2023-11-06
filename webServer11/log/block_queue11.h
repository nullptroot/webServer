#ifndef BLOCK_QUEUE_11
#define BLOCK_QUEUE_11

#include <memory>
// #include <deque>
#include <vector>
#include <mutex>
#include <condition_variable>
/*采用vector和条件变量来实现*/

template<typename T>
class block_queue
{
    private:
        std::mutex m_mutex;
        std::condition_variable m_cond;

        std::vector<T> m_array;
        
        int m_size;
        int m_front;
        int m_back;
        int m_max_size;
        bool stop;
    private:
        void init()
        {
            m_size = 0;
            m_front = -1;
            m_back = -1;
            stop = false;
        }
    public:
        block_queue(int max_size = 10000):m_size(0),m_front(-1),m_back(-1)
        {
            if(max_size <= 0)
                exit(-1);
            m_max_size = max_size;
            m_array.resize(m_max_size);
        }
        ~block_queue(){
            stop = true;
            m_cond.notify_all();
        };
        void clear()
        {
            {
                std::lock_guard<std::mutex> lk(m_mutex);
                init();
            }
        }
        bool full()
        {
            {
                std::lock_guard<std::mutex> lk(m_mutex);
                if(m_size >= m_max_size)
                    return true;
                return false;
            }
        }
        bool empty()
        {
            {
                std::lock_guard<std::mutex> lk(m_mutex);
                if(0 == m_size)
                    return true;
                return false;
            }
        }
        bool front(T &value)
        {
            {
                std::lock_guard<std::mutex> lk(m_mutex);
                if(0 == m_size)
                    return false;
                value = m_array[m_front];
                return true;
            }
        }
        bool back(T &value)
        {
            {
                std::lock_guard<std::mutex> lk(m_mutex);
                if(0 == m_size)
                    return false;
                value = m_array[m_back];
                return true;
            }
        }
        int size()
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            return m_size;
        }
        int max_size()
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            return m_max_size;
        }
        bool push(const T &item)
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if(m_size >= m_max_size)
            {
                m_cond.notify_all();
                return false;
            }
            m_back = (m_back+1)%m_max_size;
            m_array[m_back] = item;
            m_size++;
            m_cond.notify_all();
            return true;
        }
        bool pop(T &item)
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_cond.wait(lk,[this]{return m_size > 0 && stop == true;});
            if(stop == true)
                return false;
            /*没有错 是先加头再取元素的*/
            m_front = (m_front+1)%m_max_size;
            item = m_array[m_front];
            m_size--;
            return true;
        }
        bool pop(T &item,int ms_timeout)
        {
            /*设置等待时长  单位是毫秒*/
            auto t = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms_timeout);
            std::unique_lock<std::mutex> lk(m_mutex);
            m_cond.wait_until(lk,t,[this]{return m_size > 0  && stop == true;});
            if(m_size <= 0 || stop == true)
                return false;
            m_front = (m_front+1)%m_max_size;
            item = m_array[m_front];
            m_size--;
            return true;
        }
};
#endif