#pragma once
#include <functional>

#include "Channel.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Socket.h"
#include "noncopyable.h"

class Acceptor : private noncopyable {
public:
    using NewConnectionCallback =
        std::function<void(int sockfd, const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort);
    ~Acceptor();

    bool isListenning() { return m_listenning; }
    void listen();

    void setNewConnectionCallback(const NewConnectionCallback& cb) {
        m_newConnectionCallback = cb;
    }

private:
    void handleRead();

    EventLoop* m_eventLoop;
    Socket m_acceptSocket;
    Channel m_acceptChannel;
    NewConnectionCallback m_newConnectionCallback;
    bool m_listenning;
};