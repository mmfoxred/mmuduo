#include "TcpServer.h"
#include <functional>
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Logger.h"

static EventLoop* checkLoopNotNull(EventLoop* loop) {
    if (loop == nullptr) {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__,
                  __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr,
                     const std::string& name, Option option)
    : m_ipPort(listenAddr.toIpPort()),
      m_name(name),
      m_started_n(0),
      m_baseLoop(checkLoopNotNull(loop)),
      m_acceptor(new Acceptor(loop, listenAddr, option == kReusePort)),
      m_threadPool(new EventLoopThreadPool(loop, name)),
      m_nextConnId(1) {
    m_acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection,
                                                   this, std::placeholders::_1,
                                                   std::placeholders::_2));
}

TcpServer::~TcpServer() {
}

void TcpServer::set_threadNum(int n) {
    m_threadPool->set_threadNum(n);
}

void TcpServer::start() {
    if (m_started_n++ == 0) {
        m_threadPool->start(m_threadInitCallback);
        m_baseLoop->runInLoop(std::bind(&Acceptor::listen, m_acceptor.get()));
    }
}
