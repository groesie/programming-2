#include "tcp_connect.h"
#include "byte_tools.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <limits>
#include <utility>

TcpConnect::TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout,
                       std::chrono::milliseconds readTimeout)
        : ip_(ip),
          port_(port),
          connectTimeout_(connectTimeout),
          readTimeout_(readTimeout) {}

TcpConnect::~TcpConnect() {
    CloseConnection();
}

void TcpConnect::EstablishConnection() {
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

    do {
        fd_set fdSet;
        FD_ZERO(&fdSet);
        FD_SET(sock_, &fdSet);

        struct timeval tv;
        tv.tv_sec = connectTimeout_.count() / 1000;
        tv.tv_usec = (connectTimeout_.count() % 1000) * 1000;

        if (select(sock_ + 1, NULL, &fdSet, NULL, &tv) == 1) {
            int so_error;
            socklen_t length = sizeof(so_error);

            if (getsockopt(sock_, SOL_SOCKET, SO_ERROR, &so_error, &length) < 0)
                throw std::runtime_error("getsockopt() error");

            if (so_error)
                throw std::runtime_error("so_error error");

            break;
        } else
            throw std::runtime_error("Timeout exceeded\n");
    } while (true);
}

void TcpConnect::SendData(const std::string &data) const {
    if (send(sock_, data.c_str(), data.size(), 0) == -1)
        throw std::runtime_error("Can't send the data");
}

std::string TcpConnect::ReceiveData(size_t bufferSize) const {
    std::string response;

    struct pollfd fd{};
    fd.fd = sock_;
    fd.events = POLLIN;

    int ret = poll(&fd, 1, int(readTimeout_.count()));

    if (ret == -1)
        throw std::runtime_error("Cannot read\n");
    else if (ret == 0)
        throw std::runtime_error("Timeout for reading\n");

    if (bufferSize == 0) {
        char sz[4];
        long bytesRead = recv(sock_, sz, bufferSize, 0);
        if (bytesRead < 0)
            throw std::runtime_error("Cannot read\n");
        long size = ntohl(BytesToInt(sz));
        bufferSize = size;
    }

    char buffer[bufferSize];
    long bytesRead = 0;

    std::cout << "Buffer size:" << bufferSize << "\n";
    bytesRead = recv(sock_, buffer, 0, 0);
    if (bytesRead < 0)
        throw std::runtime_error("Failed to receive data");

    for (int i = 0; i < bufferSize; i++)
        response += buffer[i];

    return response;
}

void TcpConnect::CloseConnection() {
    close(sock_);
}

const std::string &TcpConnect::GetIp() const {
    return ip_;
}


int TcpConnect::GetPort() const {
    return port_;
}