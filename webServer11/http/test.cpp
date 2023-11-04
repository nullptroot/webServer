#include <string.h>
#include <iostream>
#include <string>
#include <regex>
int main()
{
    // char *temp = "wqwq\n\radda\nds";
    // char *t = strpbrk(temp+6,"\n\r");
    // printf("%p  %p  %ld\n",temp,t,t-temp);

    std::regex reg("GET");
    std::cmatch m;
    /*
    GET https://sasasas/xxx.jpg HTTP/1.1
    GET /xxx.jpg HTTP/1.1
    GET / HTTP/1.1
    
    */
    std::string temp = "GET https://sasasas/sasasas/xxx.jpg HTTP/1.1";
    int ret = std::regex_search(temp,reg);
    reg.assign("HTTP/\\d.\\d");
    ret = std::regex_search(temp.c_str(),m,reg);
    // std::regex reg("<.*>.*</.*>");
    // bool ret = std::regex_match("<html>value</html>", reg);
    bool t = m.str() == "HTTP/1.1";
    std::cout<<ret<<m.str()<<t<<std::endl;
    /*cpp不支持先行后行断言  但是可以用子表达式
    (/.+)* 若有/sasasas/ 就匹配 没有就不匹配(*)匹配任意次
    (/.*) 匹配 /文件名 (*)表示可以没有文件名只匹配 /
    m[0]全部字符串
    m[1]第一个子表达式
    m[2]第二个子表达式*/
    reg.assign("(/.+)*(/.*) ");
    ret = std::regex_search(temp.c_str(),m,reg);
    std::cout<<ret<<" "<<m[2].str()<<std::endl;

    
    /*Connection: keep-alive*/
    /*这里或的话  子表达式序号是递增的*/
    char *content = "Connection: keep-alive";
    reg.assign("(Connection:) *(keep-alive)|(Host:) *(.+:\\d+)|(Content-Length:) *(\\d+)");
    ret = std::regex_search(content,m,reg);
    std::cout<<ret<<" "<<m[2].str()<<std::endl;

    /*"Content-Length:586"*/

    char *content1 = "Content-Length:586";
    // reg.assign("(Content-Length:) *(\\d+)");
    ret = std::regex_search(content1,m,reg);
    std::cout<<ret<<" "<<m[6].str()<<std::endl;
    /*"Host: 8.141.93.140:9006"*/
    char *content2 = "Host: 8.141.93.140:9006";
    // reg.assign("(Host:) *(.+:\\d+)");
    ret = std::regex_search(content2,m,reg);
    std::cout<<ret<<" "<<m[4].str()<<std::endl;


    enum class b:std::uint8_t 
    {y = 0,t};
    int a = static_cast<int> (b::t);
    std::cout<<a<<std::endl;
    return 0;
}