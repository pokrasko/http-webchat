#include "tcp_server_socket.h"

const size_t TcpServerSocket::READ_BUFFER_SIZE = 4096;
const size_t TcpServerSocket::WRITE_BUFFER_SIZE = 4096;

TcpServerSocket::TcpServerSocket(int fd, const std::string& host, uint16_t port, Poller& poller): TcpSocket(fd, host, port, poller) {
    try {
        poller.setHandler(fd, [this](const epoll_event& event) {
            eventHandler(event);
        }, EPOLLIN);
    } catch (const std::exception& exception) {
        ::close(fd);
        throw exception;
    }
}

void TcpServerSocket::eventHandler(const epoll_event& event) {
    if (event.events & EPOLLHUP) {
        close();
        return;
    }

    if (event.events & EPOLLIN) {
        try {
            char buf[READ_BUFFER_SIZE];

            ssize_t readCount;
            bool received = false;

            while ((readCount = recv(fd, buf, READ_BUFFER_SIZE, MSG_DONTWAIT)) > 0) {
                received = true;
                inBuffer.insert(inBuffer.end(), buf, buf + readCount);
            }

            if (readCount == 0 || (readCount == -1 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                close();
                return;
            } else if (received && receivedDataHandler) {
                receivedDataHandler(inBuffer);
            }
        } catch (const std::exception& exception) {
            std::cerr << "Exception while reading from socket (fd " << fd << "), closing socket: "
                      << exception.what() << std::endl;
            close();
            return;
        }
    }

    if (event.events & EPOLLOUT) {
        try {
            char buf[WRITE_BUFFER_SIZE];
            size_t amount = std::min(outBuffer.size(), WRITE_BUFFER_SIZE);
            std::copy(outBuffer.begin(), outBuffer.begin() + amount, buf);
            ssize_t writtenCount;

            while (!outBuffer.empty() && (writtenCount = send(fd, buf, amount, MSG_DONTWAIT)) > 0) {
                outBuffer.erase(outBuffer.begin(), outBuffer.begin() + writtenCount);
                amount = std::min(outBuffer.size(), WRITE_BUFFER_SIZE);
                std::copy(outBuffer.begin(), outBuffer.begin() + amount, buf);
            }

            if (writtenCount == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                close();
                return;
            } else if (outBuffer.empty()) {
                poller.setEvents(fd, EPOLLIN);
            }
        } catch (const std::exception& exception) {
            std::cerr << "Exception while writing into socket (fd " << fd << "), closing socket: "
                      << exception.what() << std::endl;
            close();
            return;
        }
    }
}

void TcpServerSocket::setReceivedDataHandler(SocketReceivedDataHandler socketReceivedDataHandler) {
    receivedDataHandler = socketReceivedDataHandler;
    if (socketReceivedDataHandler && !inBuffer.empty()) {
        try {
            receivedDataHandler(inBuffer);
        } catch (const std::exception& exception) {
            std::cerr << "Exception while processing received data from socket (fd " << fd
                      << "), closing socket: " << exception.what() << std::endl;
            close();
        }
    }
}

void TcpServerSocket::setClosedHandler(SocketClosedHandler socketClosedHandler) {
    closedHandler = socketClosedHandler;
}

void TcpServerSocket::write(const std::string& data) {
    bool wasEmpty = outBuffer.empty();
    outBuffer.insert(outBuffer.end(), data.begin(), data.end());
    if (wasEmpty) {
        try {
            poller.setEvents(fd, EPOLLIN | EPOLLOUT);
        } catch (const std::exception& exception) {
            close();
            throw OwnException("Exception while writing to buffer of socket (fd " + std::to_string(fd) + "), closing socket: "
                  + exception.what());
        }
    }
}

TcpServerSocket::~TcpServerSocket() {
    close();
}

void TcpServerSocket::close() {
    if (closedHandler) {
        try {
            closedHandler();
        } catch (...) {}
        closedHandler = NULL;
    }
    TcpSocket::close();
}
