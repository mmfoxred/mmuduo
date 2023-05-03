#include "Socket.h"
#include "InetAddress.h"
#include "Logger.h"

#include <netinet/tcp.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

Socket::~Socket() {
    close(m_sockfd);
}

void Socket::bindAddress(const InetAddress& localaddr) {
    if (0 != ::bind(m_sockfd, (sockaddr*)localaddr.getSockAddr(),
                    sizeof(sockaddr_in))) {
        LOG_FATAL("bind sockfd:%d fail \n", m_sockfd);
    }
}

void Socket::listen() {
    if (0 != ::listen(m_sockfd, 1024)) {
        LOG_FATAL("listen sockfd:%d fail \n", m_sockfd);
    }
}

int Socket::accept(InetAddress* peeraddr) {
    /**
     * 1. accept函数的参数不合法
     * 2. 对返回的connfd没有设置非阻塞
     * Reactor模型 one loop per thread
     * poller + non-blocking IO
     */
    sockaddr_in addr;
    socklen_t len = sizeof addr;
    bzero(&addr, sizeof addr);
    int connfd = ::accept4(m_sockfd, (sockaddr*)&addr, &len,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0) {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite() {
    if (::shutdown(m_sockfd, SHUT_WR) < 0) {
        LOG_ERROR("shutdownWrite error");
    }
}

void Socket::set_tcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}

void Socket::set_reuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

void Socket::set_reusePort(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}

void Socket::set_keepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(m_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}