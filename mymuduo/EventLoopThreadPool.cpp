#include "EventLoopThreadPool.h"
#include <cstdio>
#include <memory>
#include <vector>
#include "EventLoop.h"
#include "EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop,
                                         const std::string& name)
    : m_baseLoop(baseLoop),
      m_name(name),
      m_started(false),
      m_numThread(0),
      m_next(0) {
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
    m_started = true;
    for (int i = 0; i < m_numThread; i++) {
        char buf[m_name.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", m_name.c_str(), i);
       
		//顺序地放入对应的容器中，那么每个位置的EventLoop 和 EventLoopThread 配对
		EventLoopThread* t = new EventLoopThread(cb, buf);
		m_eventLoopThreads.push_back(std::unique_ptr<EventLoopThread>(t));
        m_eventloops.push_back(t->startLoop()); //这里才是真正的创建线程的时候，会调用Thread::start()->std::thread()
    }
	//如果没有设置线程个数，那么只有mainLoop一个线程
    if (m_numThread == 0 && cb) {
        cb(m_baseLoop); //那就只能对该loop进行处理了
    }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
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

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    if (m_eventloops.empty()) {
        return {m_baseLoop};
    } else {
        return m_eventloops;
    }
}