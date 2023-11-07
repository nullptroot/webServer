#ifndef CONFIG_H
#define CONFIG_H

#include "webserver.h"
using namespace std;

class Config
{
    public:
    /*改为初始化列表形式了*/
        Config():PORT(9006),LOGWrite(0),TRIGMode(0),LISTENTrigmode(0),
                    CONNTrigmode(0),OPT_LINGER(0),sql_num(8),thread_num(8),
                    close_log(0),actor_model(1){};
        ~Config(){};
        void parse_arg(int argc,char *argv[]);
        int PORT;

        int LOGWrite;

        int TRIGMode;

        int LISTENTrigmode;

        int CONNTrigmode;

        int OPT_LINGER;

        int sql_num;

        int thread_num;

        int close_log;

        int actor_model;

};
#endif