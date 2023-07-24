#include "TcpServer.h"
#include "Logger.h"

#include <strings.h>
#include <functional>

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
	  //对::accept()得到的新连接进行处理 的回调函数 TcpServer::newConnection
    m_acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection,
                                                   this, std::placeholders::_1,
                                                   std::placeholders::_2));
}

TcpServer::~TcpServer() {
    for (auto& item : m_connections) {
        // 这个局部的shared_ptr智能指针对象，出右括号，可以自动释放new出来的TcpConnection对象资源了
        TcpConnectionPtr conn(item.second);
        item.second.reset();

        // 销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int n) {
    m_threadPool->setThreadNum(n);
}

void TcpServer::start() {
    if (m_started_n++ == 0) {
		//创建numThread个线程，并形成<EventLoop:EventLoopThread 1:1>的ThreadPool
		//并且其中的EventLoop都已经开启了事件循环 EventLoop::loop()
		//但是目前还没有事件注册，需要Acceptor接收到新连接 or 其他情况才注册事件
        m_threadPool->start(m_threadInitCallback);
		//开启::listen()，并注册poller(epoll)读事件
        m_baseLoop->runInLoop(std::bind(&Acceptor::listen, m_acceptor.get()));
    }
}


/* 
作用：
根据新连接的情况建立TcpConnection对象
对TcpConnection对象设置回调，这些回调都是给channel的、
注册该连接中channel事件到Poller中、
执行 TcpConnection::connectEstablished() 
-> channel->tie() + enableReading() + ChatServer::onConnection 输出该新连接信息
此时已经向新连接添加了读事件，也交给了一个EventLoop处理了
*/
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
    // 轮询算法，选择一个subLoop，来管理channel
    EventLoop* ioLoop = m_threadPool->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", m_ipPort.c_str(), m_nextConnId);
    ++m_nextConnId;  //这里不需要原子是因为该函数只在mainLoop中运行，不存在竞争关系
	//connName 是 TcpServer name + ipv4:port + 连接序号
	std::string connName = m_name + buf;

    // LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
    //          m_name.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0) {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    // 根据连接成功的sockfd，创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName,
                                            sockfd,  // Socket Channel
                                            localAddr, peerAddr));
    m_connections[connName] = conn;


    // 下面的回调都是用户设置给TcpServer=>TcpConnection=>Channel=>Poller=>notify channel调用回调
	//来源：ChatServer::m_server.setConnectionCallback(ChatServer::onConnection) -> TcpServer::newConnection()
	//-> conn->setConnectionCallback() 
	//-> 当执行TcpConnection::handleRead/handleWrite时会调用，不是直接传给Channel的
	//传的是是TcpConnection::handleRead/handleWrite
	conn->setConnectionCallback(m_connectionCallback);
    conn->setMessageCallback(m_messageCallback); //这个和上面的一样
    conn->setWriteCompleteCallback(m_writeCompleteCallback);
    // 设置了如何关闭连接的回调   conn->shutDown()
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    // channel->tie() enableReading() + 执行ChatServer::onConnection
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    m_baseLoop->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    // LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
    //          m_name.c_str(), conn->name().c_str());

    m_connections.erase(conn->name());
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}