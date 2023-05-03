#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Acceptor.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include "noncopyable.h"

class TcpServer : private noncopyable {
public:
    enum Option { kNoReusePort, kReusePort };

    TcpServer(EventLoop* loop, const InetAddress& listenAddr,
              const std::string& name, Option option = kNoReusePort);
    ~TcpServer();

    void setThreadNum(int n);
    void start();

    void setThreadInitcallback(const ThreadInitCallback& cb) {
        m_threadInitCallback = cb;
    }
    void setConnectionCallback(const ConnectionCallback& cb) {
        m_connectionCallback = cb;
    }
    void setMessageCallback(const MessageCallback& cb) {
        m_messageCallback = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        m_writeCompleteCallback = cb;
    }

private:
    using ConnectionMap = std::unordered_map<std::string, ConnectionPtr>;

    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const ConnectionPtr& conn);
    void removeConnectionInLoop(const ConnectionPtr& conn);

    ConnectionCallback m_connectionCallback;
    MessageCallback m_messageCallback;
    WriteCompleteCallback m_writeCompleteCallback;
    ThreadInitCallback m_threadInitCallback;

    std::string m_ipPort;
    std::string m_name;

    std::atomic_int m_started_n;
    EventLoop* m_baseLoop;  // baseLoop 用户定义的loop
    std::unique_ptr<Acceptor>
        m_acceptor;  // 运行在mainLoop，任务就是监听新连接事件
    std::shared_ptr<EventLoopThreadPool> m_threadPool;

    int m_nextConnId;
    ConnectionMap m_connections;  //保存所有连接
};