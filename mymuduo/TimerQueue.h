#pragma once

#include <mutex>
#include <set>
#include <vector>
#include "Callbacks.h"
#include "Channel.h"
#include "Timestamp.h"

class EventLoop;
class Timer;
class TimerId;

class TimerQueue : noncopyable {
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    ///
    /// Schedules the callback to be run at given time,
    /// repeats if @c interval > 0.0.
    ///
    /// Must be thread safe. Usually be called from other threads.
    TimerId addTimer(TimerCallback cb, Timestamp when, double interval);

    void cancel(TimerId timerId);

private:
    // FIXME: use unique_ptr<Timer> instead of raw pointers.
    // This requires heterogeneous comparison lookup (N3465) from C++14
    // so that we can find an T* in a set<unique_ptr<T>>.
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;
    typedef std::pair<Timer*, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    void addTimerInLoop(Timer* timer);
    void cancelInLoop(TimerId timerId);
    // called when timerfd alarms
    void handleRead();
    // move out all expired timers
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    //在两个序列中插入定时器
    bool insert(Timer* timer);

    EventLoop* loop_;
    const int
        timerfd_;  //只有一个定时器，防止同时开启多个定时器，占用多余的文件描述符
    Channel timerfdChannel_;  //定时器关心的channel对象
    // Timer list sorted by expiration
    TimerList timers_;  //定时器集合（有序）

    // for cancel()

    // activeTimerSet和timer_保存的是相同的数据
    // timers_是按照到期的时间排序的，activeTimerSet_是按照对象地址排序
    ActiveTimerSet activeTimers_;  //保存正在活动的定时器（无序）
    bool callingExpiredTimers_;    /* atomic  是否正在处理超时事件 */
    ActiveTimerSet cancelingTimers_;  //保存的是取消的定时器（无序）
};