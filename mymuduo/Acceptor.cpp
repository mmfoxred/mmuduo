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
      m_acceptSocket(createSockWithNonBlock()), //listenfd
      m_acceptChannel(loop, m_acceptSocket.get_fd()),
      m_listenning(false) {
    m_acceptSocket.set_reuseAddr(true);
    m_acceptSocket.set_reusePort(true);
    m_acceptSocket.bindAddress(listenAddr);
	//新连接到来时，需要执行回调 Acceptor::handleRead()
    m_acceptChannel.setReadCallBack(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
    m_acceptChannel.disableAll();
    m_acceptChannel.remove();
}

/*
调用次序：
Acceptor::listen() -> Socket::listen() -> ::listen()

作用：开启listen，并注册EPOLLIN事件，接收新连接事件
*/
void Acceptor::listen() {
    m_listenning = true;
    m_acceptSocket.listen();
    m_acceptChannel.enableReading();
}

/*
作用：
新连接到来时，需要执行的回调 构造函数那写了
::accept()一个新连接，同时使用TcpServer传入的m_newConnectionCallback()对该新连接进行处理

调用次序：
TcpServer::start() -> Acceptor::listen() -> ::listen() 并注册EPOLLIN事件，接收新连接事件
-> 开启新连接到来 -> Acceptor构造函数注册的 Channel::m_readCallBack
-> 实际执行的是 Acceptor::handleRead() 
-> ::accept() + TcpServer传入的m_newConnectionCallback()
-> 实际上是TcpServer::newConnection()
-> 为新连接建立TcpConnection，注册事件，输出连接信息...

作用：::accept()接收新连接，并调用TcpServer::newConnection()处理该连接
*/
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
