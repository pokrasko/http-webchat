#include "tcp_accept_socket.h"

TcpAcceptSocket::TcpAcceptSocket(Poller& poller, const std::string& host, uint16_t port, AcceptHandler acceptHandler):
        TcpSocket(poller, _m1_system_call(socket(AF_INET, SOCK_STREAM, 0),
                                          "Couldn't create the listening socket"), host, port) {
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
            } catch (std::string error) {
                std::cerr << "Couldn't accept an incoming connection: " << error << std::endl;
            } catch (std::exception& exception) {
                std::cerr << "Couldn't accept an incoming connection (Bad allocation): " << exception.what()
                          << std::endl;
            }
        }, EPOLLIN);
    } catch (std::string error) {
        ::close(fd);
        throw error;
    } catch (std::exception& exception) {
        ::close(fd);
        throw exception;
    }
}

void TcpAcceptSocket::accept(const epoll_event& event, AcceptHandler acceptHandler) {
    char hbuf[NI_MAXHOST], pbuf[NI_MAXSERV];
    uint16_t port;

    if (!(event.events & EPOLLIN)) {
        throw "Got an error epoll event while accepting socket, \"events\" = " + std::to_string(event.events);
    }

    sockaddr in = {};
    socklen_t in_len = sizeof in;
    int incomingFD = _m1_system_call(::accept(fd, &in, &in_len), "Couldn't accept an incoming connection");

    try {
        int res = getnameinfo(&in, in_len, hbuf, sizeof hbuf, pbuf, sizeof pbuf, NI_NUMERICHOST | NI_NUMERICSERV);
        if (res != 0) {
            throw "Couldn't get info about incoming connection: " + std::string(gai_strerror(res));
        }

        sscanf(pbuf, "%" SCNd16, &port);
    } catch (std::string error) {
        ::close(incomingFD);
        throw error;
    } catch (std::exception& exception) {
        ::close(incomingFD);
        throw exception;
    }

//    TcpServerSocket incomingSocket(poller, incomingFD, std::string(hbuf), port);
    acceptHandler(new TcpServerSocket(poller, incomingFD, std::string(hbuf), port));
}
