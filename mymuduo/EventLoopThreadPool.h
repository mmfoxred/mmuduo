#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : private noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string& name);
    ~EventLoopThreadPool() = default;

    void set_threadNum(int num) { m_numThread = num; }
    void start(const ThreadInitCallback & cb);
    EventLoop* get_nextLoop();
    std::vector<EventLoop*> getAllLoops();
    bool started() { return m_started; }
    const std::string get_name() const { return m_name; }

private:
    EventLoop* m_baseLoop;
    std::string m_name;
    bool m_started;
    int m_numThread;
    int m_next;
    std::vector<std::unique_ptr<EventLoopThread>> m_eventLoopThreads;
    std::vector<EventLoop*> m_eventloops;
};