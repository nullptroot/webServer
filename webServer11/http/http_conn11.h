#ifndef HTTPCONNECTION11_H
#define HTTPCONNECTION11_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <unordered_map>
#include <regex>

// #include "../locker/locker.h"
#include "../CGImysql/sql_connection_pool11.h"
#include "../timer/lst_timer.h"
#include "../log/log11.h"

class http_conn
{
    public:
        static const int FILENAME_LEN = 200;
        static const int READ_BUFFER_SIZE = 2048;
        static const int WRITE_BUFFER_SIZE = 1024;
        /*枚举类 不可以强转了嗷*/
        enum class METHOD : std::uint8_t
        {
            GET = 0,
            POST,
            HEAD,
            PUT,
            DELETE,
            TRACE,
            OPTIONS,
            CONNECT,
            PATH
        };
        static std::unordered_map<std::string,METHOD> methodMap;
        static void initMethodMap();
        enum class CHECK_STATE : std::uint8_t
        {
            CHECK_STATE_REQUESTLINE = 0,
            CHECK_STATE_HEADER,
            CHECK_STATE_CONTENT
        };
        enum class HTTP_CODE : std::uint8_t
        {
            NO_REQUEST,
            GET_REQUEST,
            BAD_REQUEST,
            NO_RESOURCE,
            FORBIDDEN_REQUEST,
            FILE_REQUEST,
            INTERNAL_ERROR,
            CLOSED_CONNECTION
        };
        enum class LINE_STATUS : std::uint8_t
        {
            LINE_OK = 0,
            LINE_BAD,
            LINE_OPEN
        };
    public:
        static int m_epollfd;
        static int m_user_count;
        connection_pool *mysql_pool;
        int m_state;
    private:
        std::unique_ptr<MYSQL,connection_pool::deleteFunc> mysql;
        int m_sockfd;
        sockaddr_in m_address;
        std::regex reg;
        char m_read_buf[READ_BUFFER_SIZE];
        long m_read_idx;
        long m_checked_idx;
        int m_start_line;
        char m_write_buf[WRITE_BUFFER_SIZE];
        int m_write_idx;
        CHECK_STATE m_check_state;
        METHOD m_method;
        std::string m_real_file;
        // char m_real_file[FILENAME_LEN];
        std::string m_url;
        std::string m_version;
        std::string m_host;
        long m_content_length;
        bool m_linger;
        char *m_file_address;
        struct stat m_file_stat;
        struct iovec m_iv[2];/*请求文件的内容和响应头在这里*/
        int m_iv_count;
        int cgi;
        std::string m_string;
        int bytes_to_send;
        int bytes_have_send;
        std::string doc_root;

        /*感觉这个变量 变为静态更好，所有对象共用一份
        下面的也需要 锁的*/
        static std::map<std::string,std::string> m_users;
        int m_TRIGMode;
        /*由于静态函数要写日志 所以这里需要是静态的*/
        static int m_close_log;

    public:
        /*下面不知可行不可行*/
        http_conn():mysql(nullptr,connection_pool::deleteFunc(nullptr)){};
        ~http_conn(){};
        /*因为类中有unique_ptr,因此当使用vector时
        会涉及copy，但是unique_ptr是不可copy的，
        因此我们只能移动*/
        http_conn(const http_conn&) = delete;
        http_conn(http_conn &&) = default;
    public:
        void init(int sockfd,const sockaddr_in &addr,std::string root,int,int,connection_pool *_mysql_pool);
        void close_conn(bool real_close = true);
        void process();
        bool read_once();
        bool write();
        sockaddr_in *get_address()
        {
            return &m_address;
        }
        /*这个函数也使用静态好一点*/
        static void initmysql_result(connection_pool *connPool);
        int timer_flag;
        int improv;
    private:
        void init();
        HTTP_CODE process_read();
        bool process_write(HTTP_CODE ret);
        HTTP_CODE parse_request_line(char *text);
        HTTP_CODE parse_headers(char *text);
        HTTP_CODE parse_content(char *text);
        HTTP_CODE do_request();
        char *get_line(){return m_read_buf + m_start_line;};
        LINE_STATUS parse_line();
        void unmap();
        bool add_response(const char *format,...);
        bool add_content(const char *content);
        bool add_status_line(int status,const char *title);
        bool add_headers(int content_length);
        bool add_content_type();
        bool add_content_length(int content_length);
        bool add_linger();
        bool add_blank_line();
};
#endif