#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

#include "Thread.h"
#include "noncopyable.h"

class EventLoop;

class EventLoopThread : private noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& initCallback,
                    const std::string& name);
    ~EventLoopThread();

    EventLoop* startLoop();

private:
    void threadFunc();

    ThreadInitCallback m_threadInitCallback;
    bool m_exiting;
    EventLoop* m_loop;
    Thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};