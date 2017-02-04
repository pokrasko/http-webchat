#ifndef HTTPWEBCHAT_TCPSOCKET_H
#define HTTPWEBCHAT_TCPSOCKET_H


#include <fcntl.h>

#include "../poller.h"

class TcpSocket {
protected:
    int fd;
    std::string host;
    uint16_t port;

    Poller& poller;
public:
    static const int NONE;

    TcpSocket(int, const std::string&, uint16_t, Poller&);
    virtual ~TcpSocket();

    virtual void close();

    bool isOpened() const;
};


#endif //HTTPWEBCHAT_TCPSOCKET_H
