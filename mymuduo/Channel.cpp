#include "Channel.h"

#include <sys/epoll.h>

#include <memory>

#include "EventLoop.h"
#include "Logger.h"

const int Channel::kNoneEvent = 0;                   // 没有注册事件
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;  // 注册了读事件
const int Channel::kWriteEvent = EPOLLOUT;           // 注册了写事件

Channel::Channel(EventLoop* loop, int fd)
    : m_loop(loop),
      m_fd(fd),
      m_events(0),
      m_revents(0),
      m_index(-1),
      m_tied(false) {}

Channel::~Channel() {
    // TODO
}

void Channel::remove() {
    m_loop->removeChannel(this);
}

/*
Caller：
EventLoop::loop()-> 调用了channel.handleEvent(m_pollReturnTime)来处理
该函数不是作为回调函数来使用的


调用次序：
Channel::handleEvent()->判断该channel是否被remove了->handleEventWithGuard()->判断事件类型，如EPOLLIN EPOLLOUT EPOLLHUP...
->调用回调函数，如m_readCallBack()...
这些回调函数不是构造函数传入的，而是外部调用Channel::setReadCallBack()传进来的，。
如 m_acceptChannel.setReadCallBack(std::bind(&Acceptor::handleRead, this)); 设置了读事件

?为什么handleRead()没有Timestamp参数也行，不是定义了ReadEventCallBack是有参数的函数嘛
*/
void Channel::handleEvent(Timestamp receiveTime) {
    if (m_tied) {
        std::shared_ptr<void> guard = m_tie.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
    } else {
        handleEventWithGuard(receiveTime);
    }
}

// 在TcpConnect建立时，会进行tie，绑定的也是TcpConnection对象指针
void Channel::tie(const std::shared_ptr<void>& obj) {
    m_tie = obj;
    m_tied = true;
}

//调用次序：Channel::update()->通过绑定的Eventloop中转到Poller EpollPoller
//->EpollPoller::updateChannel->改channel.index + EpollPoller::m_channel_map + EpollPoller::update
//->epoll_ctl实际更新，这里的m_events会在EpollPoller::update中通过get_events()读取
void Channel::update() {
    m_loop->updateChannel(this);
}

void Channel::handleEventWithGuard(Timestamp receiveTime) {
    //LOG_INFO("channel handle revents:%d\n", m_revents);
    if ((m_revents & EPOLLHUP) && !(m_revents & EPOLLIN)) {
        if (m_closeCallBack) {
            m_closeCallBack();
        }
    }
    if (m_revents & EPOLLERR) {
        if (m_errorCallBack) {
            m_errorCallBack();
        }
    }
    if (m_revents & (EPOLLIN | EPOLLPRI)) {
        if (m_readCallBack) {
            m_readCallBack(receiveTime);
        }
    }
    if (m_revents & EPOLLOUT) {
        if (m_writeCallBack) {
            m_writeCallBack();
        }
    }
}
