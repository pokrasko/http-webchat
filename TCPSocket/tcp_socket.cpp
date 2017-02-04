#include "tcp_socket.h"

const int TcpSocket::NONE = -1;

TcpSocket::TcpSocket(int fd, const std::string& host, uint16_t port, Poller& poller): fd(fd), host(host), port(port), poller(poller) {
    try {
        std::string error = "Couldn't make socket (fd " + std::to_string(fd) + ") non-blocking";
        int flags = _m1_system_call(fcntl(fd, F_GETFL, 0), error);
        _m1_system_call(fcntl(fd, F_SETFL, flags | O_NONBLOCK), error);
    } catch (const std::exception& exception) {
        ::close(fd);
        throw exception;
    }
}

TcpSocket::~TcpSocket() {
    close();
}

void TcpSocket::close() {
    if (fd != NONE) {
        try {
            poller.removeHandler(fd);
            _m1_system_call(::close(fd), "Socket (fd " + std::to_string(fd) + ") was closed incorrectly");
        } catch (const std::exception& exception) {
            std::cerr << "Exception while closing socket (fd " << fd << "): " << exception.what() << std::endl;
        }
        fd = NONE;
    }
}

bool TcpSocket::isOpened() const {
    return fd != NONE;
}
