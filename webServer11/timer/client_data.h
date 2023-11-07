#ifndef CLIENT_DATA_H
#define CLIENT_DATA_H
#include <netinet/in.h>

template <typename T>
struct client_data
{
    sockaddr_in address;
    int sockfd;
    T *timer;
};
#endif