#pragma once

#include "noncopyable.h"

class InetAddress;

// 封装socket fd
class Socket : noncopyable {
public:
    explicit Socket(int sockfd) : m_sockfd(sockfd) {}

    ~Socket();

    int get_fd() const { return m_sockfd; }
    void bindAddress(const InetAddress& localaddr);
    void listen();
    int accept(InetAddress* peeraddr);

    void shutdownWrite();

    void set_tcpNoDelay(bool on);
    void set_reuseAddr(bool on);
    void set_reusePort(bool on);
    void set_keepAlive(bool on);

private:
    const int m_sockfd;
};