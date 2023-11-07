#ifndef MIN_HEAP_H
#define MIN_HEAP_H
#include <iostream>
#include <netinet/in.h>
#include <time.h>
#include "client_data.h"

using std::exception;
/*这里是对 client_data成员缓冲区的大小设定*/
// #define BUFFER_SIZE 64;

class heap_timer
{
    public:
        time_t expire;
        void(*cb_func)(client_data<heap_timer> *);
        client_data<heap_timer> *user_data;
};
class time_heap
{
    private:
        heap_timer **array;
        int capacity;
        int cur_size;
    public:
        time_heap(int cap);
        time_heap(heap_timer **init_array,int size,int capacity);
        ~time_heap();
        void add_timer(heap_timer *timer);
        void del_timer(heap_timer *timer);
        heap_timer *top() const;
        void pop_timer();
        void tick();
        bool empty() const;
        /*需要加一个调整的方法喽*/
        void adjust_timer(heap_timer *timer);
    private:
        void percolate_down(int hole);
        void resize();
};
#endif