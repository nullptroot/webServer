// #ifndef LOG_H
// #define LOG_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdarg.h>
#include "block_queue11.h"
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
class Log
{
    private:
        std::string dir_name;
        std::string log_name;
        std::vector<std::string> map;
        int m_split_lines;
        int m_log_buf_size;
        long long m_count;
        int m_today;
        std::fstream m_fp;
        /*原代码根本就没有释放啊 内存泄露了*/
        std::string m_buf;
        // std::unique_ptr<char,void(*)(char *)> m_buf;
        // char *m_buf;
        /*原代码根本就没有释放啊 内存泄露了*/
        std::unique_ptr<block_queue<std::string>> m_log_queue;
        // block_queue<std::string> *m_log_queue;
        bool m_is_async;
        std::mutex m_mutex;
        int m_closs_log;
    public:
        static Log *get_instance()
        {
            static Log instance;
            return &instance;
        }
        void flush_log_thread()
        {
            Log::get_instance()->async_write_log();
        }
        bool init(const std::string &file_name,int close_log,int log_buf_size = 8192,
                    int split_lines = 5000000,int max_queue_size = 0);
        void write_log(int level,const char *format,...);
        void flush(void);
    private:
        Log();
        virtual ~Log();
        void *async_write_log()
        {
            std::string single_log;
            while(m_log_queue->pop(single_log))
            {
                std::lock_guard<std::mutex> lk(m_mutex);
                m_fp<<single_log;
            }
            return nullptr;
        }

};
/*
LOG_DEBUG(format, ...)
展开为如下，感觉不如内联函数好用
if(0 == m_closs_log)
{
    Log::get_instance()->write_log(0,format,##__VA_ARGS__);
    Log::get_instance()->flush();
}
*/
#define LOG_DEBUG(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(0,format,##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(1,format,##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(2,format,##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(3,format,##__VA_ARGS__); Log::get_instance()->flush();}

// #endif