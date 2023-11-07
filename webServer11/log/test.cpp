#include <string>
#include <iostream>
#include <stdarg.h>
// using namespace std;
using std::string;
using std::is_same;
using std::is_constructible;
using std::cout;
using std::endl;
void fun(string &s){}

template <typename T,typename... Args>
void fun(string &result,const T &t,const Args&... args)
{
    if(is_same<string,T>::value)
    {
        result += t;
        cout<<"true"<<" ";
    }
    else
    {
        cout<<"false:  "<<is_constructible<int,T>::value<<" ";
        result += std::to_string((int)t);
    }
    fun(result,args...);
}
template <typename T>
void f(const T &s)
{
    if(is_same<string,T>::value)
        cout<<"true"<<endl;
    else    
        cout<<"false"<<endl;
}

void func1(const char *format,...)
{
    va_list valist;
    va_start(valist,format);
    printf(format,valist);
    // cout<<valist<<endl;
    va_end(valist);
}
int main()
{
    // string s = "";
    // const auto& b = string("assas");
    // f(b);
    // cout<<endl;
    // fun(s,b,18,60,9.8);
    // cout<<endl;
    // cout<<s<<endl;
    // func1("%d  %s  %s",8,"wqwqw","wqedf");
    std::string s = "";
    s.resize(100);
    sprintf(s.c_str(),"%d,%d",10,8);
}