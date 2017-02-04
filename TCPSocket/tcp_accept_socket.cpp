#include "tcp_accept_socket.h"

TcpAcceptSocket::TcpAcceptSocket(const std::string& host, uint16_t port, AcceptHandler acceptHandler, Poller& poller):
        TcpSocket(_m1_system_call(socket(AF_INET, SOCK_STREAM, 0),
                                          "Couldn't create the listening socket"), host, port, poller) {
    try {
        int opt = 1;
        _m1_system_call(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt),
                        "Couldn't make the listening socket reusable");

        sockaddr_in sa = {};
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = INADDR_ANY;
        sa.sin_port = htons(port);
        _m1_system_call(bind(fd, (sockaddr*) &sa, sizeof sa), "Couldn't bind the listening socket");

        _m1_system_call(listen(fd, SOMAXCONN), "Couldn't execute listen on the listening socket");

        poller.setHandler(fd, [=](epoll_event event) {
            try {
                accept(event, acceptHandler);
            } catch (const std::exception& exception) {
                std::cerr << "Couldn't accept an incoming connection: " << exception.what() << std::endl;
            }
        }, EPOLLIN);
    } catch (const std::exception& exception) {
        ::close(fd);
        throw exception;
    }
}

void TcpAcceptSocket::accept(const epoll_event& event, AcceptHandler acceptHandler) {
    char hbuf[NI_MAXHOST], pbuf[NI_MAXSERV];
    uint16_t port;

    if (!(event.events & EPOLLIN)) {
        throw OwnException("Got an error epoll event while accepting socket, \"events\" = "
                                 + std::to_string(event.events));
    }

    sockaddr in = {};
    socklen_t in_len = sizeof in;
    int incomingFD = _m1_system_call(::accept(fd, &in, &in_len), "Couldn't accept an incoming connection");

    try {
        int res = getnameinfo(&in, in_len, hbuf, sizeof hbuf, pbuf, sizeof pbuf, NI_NUMERICHOST | NI_NUMERICSERV);
        if (res != 0) {
            throw OwnException("Couldn't get info about incoming connection: " + std::string(gai_strerror(res)));
        }

        sscanf(pbuf, "%" SCNu16, &port);
    } catch (const std::exception& exception) {
        ::close(incomingFD);
        throw exception;
    }

    acceptHandler(new TcpServerSocket(incomingFD, std::string(hbuf), port, poller));
}
