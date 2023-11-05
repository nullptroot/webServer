#include "log11.h"
#include <unistd.h>
#include <fstream>
#include <memory>
#include <condition_variable>
#include <pthread.h>
/*大概是找到阻塞的原因了，析构的时候，阻塞
在block_queue的析构函数了应该

确实找到了，block_queue对象要销毁的时候
要进入析构的时候，此时可能有线程会阻塞在
pop函数调用上，这时候就会发生阻塞了，看
下面的测试代码，也就是条件变量阻塞等待的时候
是析构不了的*/
void *func(void *a)
{
    while(1)
    {
        std::cout<<"asasa\n";
        sleep(2);
    }
}
struct ts
{
    std::condition_variable m_cond;
    void wait1()
    {
        std::mutex m_mutex;
        std::unique_lock<std::mutex> lk(m_mutex);
        m_cond.wait(lk,[](){return 0;});
    }
};
int main()
{
    // Log::get_instance()->init("./ServerLog", 0, 2000, 800000, 2);
    // Log::get_instance()->write_log(1,"%s","sasasaa");
    // Log::get_instance()->write_log(1,"%s","sasasaa");
    // std::cout<<"1\n";
    // std::cout<<"2\n";
    ts a;
    std::thread b(&ts::wait1,&a);
    b.detach();
    sleep(2);

    return 0;
}