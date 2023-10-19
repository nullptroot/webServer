#include <list>
#include <exception>
#include <thread>
#include <mutex>
#include <semaphore>
#include <memory>
#include "../locker/locker.h"
template<typename T>
class Deleter{
public:
	void operator()(T *ptr)const{
		delete []ptr;
	}
};
int main()
{
    std::unique_ptr<std::thread,Deleter<std::thread>> m_threads;
    return 0;
}