#include "byte_tools.h"
#include "tcp_connect.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <stdexcept>

TcpConnect::TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout, std::chrono::milliseconds readTimeout)
    : ip_(std::move(ip)), port_(port), connectTimeout_(connectTimeout), readTimeout_(readTimeout), sock_(-1) {}

TcpConnect::~TcpConnect() {
    if (sock_ >= 0) {
        CloseConnection();
    }
}

void TcpConnect::EstablishConnection() {
    // sock_ = socket(AF_INET, SOCK_STREAM, 0);
    // if (sock_ < 0) {
    //     throw std::runtime_error(std::string("Error creating socket: ") + strerror(errno));
    // }

    // struct sockaddr_in addr;
    // addr.sin_family = AF_INET;
    // addr.sin_port = htons(port_);
    // inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr);

    // int flags = fcntl(sock_, F_GETFL, 0);
    // fcntl(sock_, F_SETFL, flags | O_NONBLOCK);

    // int res = connect(sock_, (struct sockaddr*)&addr, sizeof(addr));
    // if (res < 0 && errno != EINPROGRESS) {
    //     throw std::runtime_error(std::string("Error connecting to the server: ") + strerror(errno));
    // }

    // struct pollfd pfd;
    // pfd.fd = sock_;
    // pfd.events = POLLOUT;

    // int poll_res = poll(&pfd, 1, connectTimeout_.count());
    // if (poll_res <= 0) {
    //     CloseConnection();
    //     throw std::runtime_error("Connection timeout");
    // }

    // fcntl(sock_, F_SETFL, flags);

    // int error = 0;
    // socklen_t error_len = sizeof(error);
    // getsockopt(sock_, SOL_SOCKET, SO_ERROR, &error, &error_len);
    // if (error != 0) {
    //     throw std::runtime_error(std::string("Error during connection: ") + strerror(error));
    // }
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == -1)
        throw std::runtime_error("Can't create a socket!\n");

    struct sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port_);
    if(inet_pton(AF_INET, ip_.c_str(), &hint.sin_addr) <= 0)
        throw std::runtime_error("Invalid IP address\n");

    int isOn = 1;
    setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &isOn, sizeof(int));

    if (fcntl(sock_, F_SETFL, O_NONBLOCK) == -1)
        throw std::runtime_error("Cannot set to nonblocking mode\n");

    if (connect(sock_, (struct sockaddr *) &hint, sizeof(hint)) == -1) {
        if (errno != EINPROGRESS) {
           throw std::runtime_error("Can't connect to IP/port\n");
        }
    }

    fd_set fdSet;
    FD_ZERO(&fdSet);
    FD_SET(sock_, &fdSet);

    struct timeval tv;
    tv.tv_usec = connectTimeout_.count();

    if (select(sock_ + 1, NULL, &fdSet, NULL, &tv) == 1)
    {
        int so_error;
        socklen_t length = sizeof(so_error);

        getsockopt(sock_, SOL_SOCKET, SO_ERROR, &so_error, &length);

        if (so_error == 0) {
            int fgs = fcntl(sock_, F_GETFL, 0);
            fgs = fgs & ~O_NONBLOCK;
            fcntl(sock_, F_SETFL, fgs);
            return;
        }
    }
    throw std::runtime_error("Timeout exceeded\n");
}

void TcpConnect::SendData(const std::string& data) const {
    ssize_t sent_bytes = send(sock_, data.c_str(), data.size(), 0);
    if (sent_bytes < 0) {
        throw std::runtime_error(std::string("Error sending data: ") + strerror(errno));
    }
}

std::string TcpConnect::ReceiveData(size_t bufferSize) const {
    struct pollfd pfd;
    pfd.fd = sock_;
    pfd.events = POLLIN;

    int poll_res = poll(&pfd, 1, readTimeout_.count());
    if (poll_res <= 0) {
        throw std::runtime_error("Read timeout");
    }

    if (bufferSize == 0) {
        uint32_t msg_size;
        ssize_t recv_bytes = recv(sock_, &msg_size, sizeof(msg_size), 0);
        if (recv_bytes < 0) {
            throw std::runtime_error(std::string("Error receiving data: ") + strerror(errno));
        }
        bufferSize = ntohl(msg_size);
    }

    std::string buffer(bufferSize, '\0');
    size_t recv_total = 0;
    size_t cur = 0;
    auto startTime = std::chrono::steady_clock::now();
    do{
        auto diff = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::milliseconds> (diff) > readTimeout_ / 2) {
            throw std::runtime_error("Read timeout");
        }

        ssize_t recv_bytes = recv(sock_, &buffer[cur], bufferSize, 0);
        if (recv_bytes < 0) {
            throw std::runtime_error(std::string("Error receiving data: ") + strerror(errno));
        }
        recv_total += recv_bytes;
        cur = recv_total;
    } while (recv_total < bufferSize);
    

    buffer.resize(recv_total);
    return buffer;
}

void TcpConnect::CloseConnection() {
    if (sock_ >= 0) {
        close(sock_);
        sock_ = -1;
    }
}

const std::string& TcpConnect::GetIp() const {
    return ip_;
}

int TcpConnect::GetPort() const {
    return port_;
}