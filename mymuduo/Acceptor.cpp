#include "Acceptor.h"
#include <sys/socket.h>
#include <unistd.h>
#include <functional>
#include "InetAddress.h"
#include "Logger.h"

static int createSockWithNonBlock() {
    int sockfd =
        ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) {
        LOG_FATAL("%s:%s:%d listenSocket create err%d\n", __FILE__,
                  __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr,
                   bool reusePort)
    : m_eventLoop(loop),
      m_acceptSocket(createSockWithNonBlock()),
      m_acceptChannel(loop, m_acceptSocket.get_fd()),
      m_listenning(false) {
    m_acceptSocket.set_reuseAddr(true);
    m_acceptSocket.set_reusePort(true);
    m_acceptSocket.bindAddress(listenAddr);
    m_acceptChannel.setReadCallBack(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
    m_acceptChannel.disableAll();
    m_acceptChannel.remove();
}

void Acceptor::listen() {
    m_listenning = true;
    m_acceptSocket.listen();
    m_acceptChannel.enableReading();
}

void Acceptor::handleRead() {
    InetAddress peerAddr;
    int connfd = m_acceptSocket.accept(&peerAddr);
    if (connfd >= 0) {
        if (m_newConnectionCallback) {
            m_newConnectionCallback(connfd, peerAddr);
        } else {
            ::close(connfd);
        }
    } else {
        LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__,
                  errno);
        if (errno == EMFILE) {
            LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__,
                      __FUNCTION__, __LINE__);
        }
    }
}
