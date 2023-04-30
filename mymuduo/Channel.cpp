#include "Channel.h"

#include <sys/epoll.h>

#include <memory>

#include "EventLoop.h"
#include "Logger.h"

Channel::Channel(EventLoop* loop, int fd)
    : m_loop(loop),
      m_fd(fd),
      m_events(0),
      m_revents(0),
      m_index(0),
      m_tied(false) {}

Channel::~Channel() {
    // TODO
}

void Channel::remove() {
    m_loop->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime) {
    if (m_tied) {
        std::shared_ptr<void> guard = m_tie.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
    } else {
        handleEvent(receiveTime);
    }
}

// 在TcpConnect建立时，会进行tie
void Channel::tie(const std::shared_ptr<void>& obj) {
    m_tie = obj;
    m_tied = true;
}

void Channel::update() {
    m_loop->updateChannel(this);
}

void Channel::handleEventWithGuard(Timestamp receiveTime) {
    LOG_INFO("channel handle revents:%d\n", m_revents);
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
