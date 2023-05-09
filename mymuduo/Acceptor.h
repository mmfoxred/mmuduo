#pragma once
#include <functional>

#include "Channel.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Socket.h"
#include "noncopyable.h"

//接收新连接，始终运行在mainLoop中。
//算一种特殊的fd（listenfd）需要EventLoop处理
class Acceptor : private noncopyable {
public:
    using NewConnectionCallback =
        std::function<void(int sockfd, const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort);
    ~Acceptor();

    bool isListenning() { return m_listenning; }
    void listen();

	//在TcpServer构造函数中调用
    void setNewConnectionCallback(const NewConnectionCallback& cb) {
        m_newConnectionCallback = cb;
    }

private:
    void handleRead();

    EventLoop* m_eventLoop;
    Socket m_acceptSocket;
    Channel m_acceptChannel;
    NewConnectionCallback m_newConnectionCallback; //是TcpServer::newConnection
    bool m_listenning;
};