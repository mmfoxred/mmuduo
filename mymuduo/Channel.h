#pragma once
#include <sys/epoll.h>

#include <functional>
#include <memory>
#include <utility>

#include "Timestamp.h"
#include "noncopyable.h"

class EventLoop;

/*
Channel：
    1.封装成感兴趣的事件，<fd:events>；
    可以读取/设置事件,但不设置到内核epoll事件集合中，设置要交给Poller进行epoll_ctl
    2.处理Poller返回的事件 Poller->EventLoop->Channel
*/
class Channel : private noncopyable {
public:
    using EventCallBack = std::function<void()>;
    using ReadEventCallBack = std::function<void(Timestamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();
    void remove();

    // Channel得到Poller的事件返回后，进行处理
    void handleEvent(Timestamp receiveTime);
    /*来源：
    ChatServer::自己编的回调函数          -> TcpServer::set...Callback
    -> TcpServer::newConnection() -> conn.setConnectionCallback()
	-> TcpConnection构造函数就会设置
    */
    void setReadCallBack(ReadEventCallBack cb) { m_readCallBack = std::move(cb); }
    void setWriteCallBack(EventCallBack cb) { m_writeCallBack = std::move(cb); }
    void setErrorCallBack(EventCallBack cb) { m_errorCallBack = std::move(cb); }
    void setCloseCallBack(EventCallBack cb) { m_closeCallBack = std::move(cb); }
    // 监听Channel是否被remove掉了
    void tie(const std::shared_ptr<void>&);
    // 获取信息 设置信息
    int get_fd() const { return this->m_fd; }
    int get_events() const { return this->m_events; }
    void set_revents(int revents) { m_revents = revents; }
    int get_index() const { return this->m_index; }
    void set_index(int index) { m_index = index; }
    EventLoop* ownerLoop() const { return this->m_loop; }

	// 设置fd的事件
    void enableReading() {
        m_events |= kReadEvent;
        update(); //注明了调用次序
    }
    void disableReading() {
        m_events &= ~kReadEvent;
        update();
    }
    void enableWriting() {
        m_events |= kWriteEvent;
        update();
    }
    void disableWriting() {
        m_events &= ~kWriteEvent;
        update();
    }
    void disableAll() {
        m_events = kNoneEvent;
        update();
    }
	
    // 获取fd的事件
    bool isNoneEvent() { return m_events == kNoneEvent; }
    bool isReading() { return m_events & kReadEvent; }
    bool isWriting() { return m_events & kWriteEvent; }

private:
	
    void update(); 
    void handleEventWithGuard(Timestamp receiveTime);

    // 事件监听相关
    const static int kNoneEvent;                   // 没有注册事件
    const static int kReadEvent;  // 注册了读事件
    const static int kWriteEvent;           // 注册了写事件

    EventLoop* m_loop;
    const int m_fd;  // 要监听的fd
    int m_events;    // 感兴趣的事件
    int m_revents;   // Poller中返回的发生的事件
    int m_index;

    // 监听该是否channel是否被手动remove了
    std::weak_ptr<void> m_tie;
    bool m_tied;

    // 事件回调函数
    ReadEventCallBack m_readCallBack;
    EventCallBack m_writeCallBack;
    EventCallBack m_errorCallBack;
    EventCallBack m_closeCallBack;
};