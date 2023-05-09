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

	//ChatServer中没有使用
    void setThreadInitcallback(const ThreadInitCallback& cb) {
        m_threadInitCallback = cb;
    }
	//ChatServer中使用了,设置了ChatServer::onConnection
    void setConnectionCallback(const ConnectionCallback& cb) {
        m_connectionCallback = cb;
    }
	//ChatServer中使用了，设置了ChatServer::onMessage
    void setMessageCallback(const MessageCallback& cb) {
        m_messageCallback = cb;
    }
	//ChatServer中没有使用
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        m_writeCompleteCallback = cb;
    }

private:
    using ConnectionMap = std::unordered_map<std::string, ConnectionPtr>;

    void newConnection(int sockfd, const InetAddress& peerAddr); //给Acceptor的setNewConnectionCallback
    void removeConnection(const ConnectionPtr& conn);
    void removeConnectionInLoop(const ConnectionPtr& conn);

    ConnectionCallback m_connectionCallback;//给TcpConnection初始化的
    MessageCallback m_messageCallback;//给TcpConnection初始化的
	//ChatServer中没有使用的回调
	WriteCompleteCallback m_writeCompleteCallback; //给TcpConnection初始化的
    ThreadInitCallback m_threadInitCallback; //给线程池EventLoopThreadPool初始化的，逐级传递，用来在线程初始化（底层std::thread)时做的事情

    std::string m_ipPort;
    std::string m_name;

    std::atomic_int m_started_n;
    EventLoop* m_baseLoop;  // baseLoop 用户定义的loop
    std::unique_ptr<Acceptor>
        m_acceptor;  // 运行在mainLoop，任务就是监听新连接事件
    std::shared_ptr<EventLoopThreadPool> m_threadPool;

    int m_nextConnId; //来一个新连接，该值增1，连接断开时不会减少
    ConnectionMap m_connections;  //保存所有连接
};