#include "CircularBuffer.h"
#include "CurrentThread.h"
#include "Logger.h"
#include "TcpServer.h"

#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
using namespace std;
using namespace placeholders;

class ChatServer {
public:
    ChatServer(EventLoop* loop, const InetAddress& listenAddr,
               const string& nameArg, int threadCount)
        : m_server(loop, listenAddr, nameArg), m_loop(loop) {
        //注册用户连接和断开的回调
        m_server.setConnectionCallback(
            std::bind(&ChatServer::onConnection, this, _1));
        //注册用户读写事件的回调
        m_server.setMessageCallback(
            std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        //线程数 1个IO线程，3个work线程
        m_server.setThreadNum(threadCount);
    }
    void start() { m_server.start(); }

private:
    // 用户连接、断开事件的回调 epoll_wait listenfd accept
    void onConnection(const TcpConnectionPtr& conn) {
        conn->setTcpNoDelay(true);
        // if (conn->connected()) {
        //     cout << conn->peerAddress().toIpPort() << "->"
        //          << conn->localAddress().toIpPort() << " state:online " << endl;
        // } else {
        //     cout << conn->peerAddress().toIpPort() << "->"
        //          << conn->localAddress().toIpPort() << " state:offline "
        //          << endl;
        //     conn->shutdown();
        //     // m_loop->quit();
        // }
    }

    //服务器处理的事件：读写事件，其中除了新连接（mainLoop中运行）
    //还有普通读写事件需要处理，那就交给这里了（subLoop）
    //用户读写事件的回调
    void onMessage(const TcpConnectionPtr& conn, CircularBuffer* buffer,
                   Timestamp time) {
        conn->send(buffer->retrieveAllAsString());
    }
    TcpServer m_server;
    EventLoop* m_loop;  //mainLoop
};

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: server <address> <port> <threads>\n");
        exit(0);
    }
    ////LOG_INFO("pid = %d, tid = %d\n", getpid(), CurrentThread::getTid());
    const char* ip = argv[1];
    short port = static_cast<short>(atoi(argv[2]));
    InetAddress listenAddr(ip, port);
    int threadCount = atoi(argv[3]);

    EventLoop loop;              //主事件循环 mainLoop
    InetAddress addr(ip, port);  //服务器要监听的地址
    ChatServer server(&loop, addr, "PingPong", threadCount);
    server.start();
    loop.loop();  //相当于epoll_wait
    return 0;
}