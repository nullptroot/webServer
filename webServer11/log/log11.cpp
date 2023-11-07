#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log11.h"
#include <pthread.h>
#include <thread>
#include <chrono>
#include <unistd.h>
#define LOG_VAL_PARM 1024

Log::Log()
{
    m_count = 0;
    m_is_async = false;
}
Log::~Log()
{
    m_fp.close();
}

bool Log::init(const std::string &file_name,int close_log,int log_buf_size,int split_lines,int max_queue_size)
{
    if(max_queue_size >= 1)
    {
        m_is_async = true;
        m_log_queue.reset(new block_queue<std::string>(max_queue_size));
        std::thread tid = std::thread(&Log::flush_log_thread,this);
        tid.detach();
    }
    map.emplace_back("[debug]:");
    map.emplace_back("[info]:");
    map.emplace_back("[warn]:");
    map.emplace_back("[error]:");

    m_closs_log = close_log;
    m_log_buf_size = log_buf_size;
    // m_buf.reset(new char[m_log_buf_size]);
    m_buf.resize(m_log_buf_size);
    m_split_lines = split_lines;

    auto tim = std::chrono::system_clock::now();
    
    time_t t = std::chrono::system_clock::to_time_t(tim);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    int index = file_name.find('/');
    log_name = file_name.substr(index+1);
    std::string log_full_name;
    if(index == log_full_name.size())
        log_full_name += std::to_string(my_tm.tm_year+1900)
                        +"_"+std::to_string(my_tm.tm_mon+1)
                        +"_"+std::to_string(my_tm.tm_mday);
    else
    {
        dir_name = file_name.substr(0,index+1);
        log_full_name += dir_name + std::to_string(my_tm.tm_year+1900)
                        +"_"+std::to_string(my_tm.tm_mon+1)
                        +"_"+std::to_string(my_tm.tm_mday)
                        +"_"+ log_name;
    }
    
    m_today = my_tm.tm_mday;

    m_fp.open(log_full_name,std::fstream::out);
    

    if(!m_fp.is_open())
        return false;
    return true;
}
void Log::write_log(int level,const char *format, ...)
{
    struct timeval now = {0,0};
    gettimeofday(&now,nullptr);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    std::unique_lock<std::mutex> lk(m_mutex);
    m_count++;

    if(m_today != my_tm.tm_mday || m_count % m_split_lines == 0)
    {
        /*新日志名字*/
        std::string new_log;
        m_fp.flush();
        m_fp.close();

        std::string tail = std::to_string(my_tm.tm_year+1900)+"_"
                            +std::to_string(my_tm.tm_mon+1)+"_"
                            +std::to_string(my_tm.tm_mday) +"_";

        if(m_today != my_tm.tm_mday)
        {
            new_log = dir_name+tail+log_name;
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else
            new_log = dir_name+tail+log_name + std::to_string(m_count/m_split_lines);
        m_fp.open(new_log,std::fstream::out);
    }
    lk.unlock();

    va_list valist;
    va_start(valist,format);

    std::string log_str;
    lk.lock();
    m_buf.clear();
    /*这里使得每条日志都带上了时间*/
    m_buf += std::to_string(my_tm.tm_year + 1900)+'-'
                +std::to_string(my_tm.tm_mon + 1)+'-'
                +std::to_string(my_tm.tm_mday)+' '
                +std::to_string(my_tm.tm_hour)+':'
                +std::to_string(my_tm.tm_min)+':'
                +std::to_string(my_tm.tm_sec)+'.'
                +std::to_string(now.tv_usec)+' '
                +map[level] + ' ';
    // int n = snprintf(m_buf.get(),48,"%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
    //                  my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
    //                  my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, map[level].c_str());
    char temp[LOG_VAL_PARM];
    /*变长参数解决不了了
    边长模板用不了擦*/
    int m = vsnprintf(temp,LOG_VAL_PARM-1,format,valist);
    m_buf += temp;
    log_str = m_buf;
    // log_str += '\n';

    lk.unlock();

    if(m_is_async && !m_log_queue->full())
        m_log_queue->push(log_str);
    else
    {
        lk.lock();
        m_fp<<log_str<<std::endl;
        lk.unlock();
    }
    va_end(valist);
}

void Log::flush(void)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_fp.flush();
}