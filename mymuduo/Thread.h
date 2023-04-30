#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "noncopyable.h"

class Thread : private noncopyable {
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();

    bool isStarted() const { return m_started; }

    pid_t get_tid() const { return m_tid; }

    const std::string& get_name() const { return m_name; }

    static int get_numCount() { return m_numCount; }

private:
    void setDefaultName();

    bool m_started;
    bool m_joined;
    std::shared_ptr<std::thread> m_thread;
    pid_t m_tid;
    std::string m_name;
    static std::atomic_int m_numCount;
    ThreadFunc m_funcs;
};