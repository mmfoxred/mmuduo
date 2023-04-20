#include <muduo/base/Timestamp.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <functional>
#include <iostream>
#include <string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

class ChatServer {
public:
    ChatServer(EventLoop* loop, const InetAddress& listenAddr,
               const string& nameArg)
        : m_server(loop, listenAddr, nameArg), m_loop(loop) {
        //注册用户连接和断开的回调
        m_server.setConnectionCallback(
            std::bind(&ChatServer::onConnection, this, _1));
        //注册用户读写时间的回调
        m_server.setMessageCallback(
            std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        //线程数 1个IO线程，3个work线程
        m_server.setThreadNum(4);
    }
    void start() { m_server.start(); }

private:
    // 用户连接、断开事件的回调 epoll_wait listenfd accept
    void onConnection(const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            cout << conn->peerAddress().toIpPort() << "->"
                 << conn->localAddress().toIpPort() << " state:online " << endl;
        } else {
            cout << conn->peerAddress().toIpPort() << "->"
                 << conn->localAddress().toIpPort() << " state:offline "
                 << endl;
            conn->shutdown();
            // m_loop->quit();
        }
    }
    //用户读写事件的回调 epoll_wait sockfd recv send
    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer,
                   Timestamp time) {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data: " << buf << " time: " << time.toString() << endl;
        conn->send(buf);
    }
    TcpServer m_server;
    EventLoop* m_loop;
};

int main() {
    EventLoop loop;
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();  //相当于epoll_wait
    return 0;
}