#pragma once
#include <atomic>
#include "Callbacks.h"
#include "Timestamp.h"
#include "noncopyable.h"

class Timer : noncopyable {
public:
    Timer(TimerCallback cb, Timestamp when, double interval)
        : callback_(std::move(cb)),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequence_(s_numCreated_++) {}

    void run() const { callback_(); }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    void restart(Timestamp now);

    static int64_t numCreated() { return s_numCreated_; }

private:
    const TimerCallback callback_;
    Timestamp expiration_;    //定时器过期时间.
    const double interval_;   //周期性定时时间.
    const bool repeat_;       //是否是周期性定时.
    const int64_t sequence_;  //定时器的序号？

    //用于记录Timer创建的个数.
    //为了保证它的线程安全性，使用AtomicInt64封装了一层原子操作.
    static std::atomic<int> s_numCreated_;
};