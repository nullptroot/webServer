#include "log11.h"
#include <unistd.h>
#include <fstream>
#include <memory>
#include <pthread.h>
void *func(void *a)
{
    while(1)
    {
        std::cout<<"asasa\n";
        sleep(2);
    }
}
int main()
{
    Log::get_instance()->init("./ServerLog", 0, 2000, 800000, 2);
    Log::get_instance()->write_log(1,"%s","sasasaa");
    Log::get_instance()->write_log(1,"%s","sasasaa");
    std::cout<<"1\n";
    std::cout<<"2\n";
    sleep(2);

    return 0;
}