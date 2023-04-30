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
    void threadFunc();  //线程需要执行的函数，作为线程的初始化变量

    ThreadInitCallback m_threadInitCallback;  //好像是给EventLoop做其他初始化的
    bool m_exiting;
    EventLoop* m_loop;
    Thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};