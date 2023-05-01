#include "EventLoopThreadPool.h"
#include <cstdio>
#include <memory>
#include <vector>
#include "EventLoop.h"
#include "EventLoopThread.h"

EventLoopThreadLoop::EventLoopThreadLoop(EventLoop* baseLoop,
                                         const std::string& name)
    : m_baseLoop(baseLoop),
      m_name(name),
      m_started(false),
      m_numThread(0),
      m_next(0) {}

void EventLoopThreadLoop::start(const ThreadInitCallback& cb) {
    m_started = true;
    for (int i = 0; i < m_numThread; i++) {
        char buf[m_name.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", m_name.c_str(), i);
        EventLoopThread* t = new EventLoopThread(cb, buf);
        m_eventLoopThreads.push_back(std::unique_ptr<EventLoopThread>(t));
        m_eventloops.push_back(t->startLoop());
    }
    if (m_numThread == 0 && cb) {
        cb(m_baseLoop);
    }
}

EventLoop* EventLoopThreadLoop::get_nextLoop() {
    EventLoop* tmp_loop = m_baseLoop;
    if (!m_eventloops.empty()) {
        tmp_loop = m_eventloops[m_next];
        ++m_next;
        if (m_next >= m_eventloops.size()) {
            m_next = 0;
        }
    }
    return tmp_loop;
}

std::vector<EventLoop*> EventLoopThreadLoop::getAllLoops() {
    if (m_eventloops.empty()) {
        return {m_baseLoop};
    } else {
        return m_eventloops;
    }
}