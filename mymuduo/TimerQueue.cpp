#include <assert.h>
#include <strings.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "EventLoop.h"
#include "Logger.h"
#include "Timer.h"
#include "TimerId.h"
#include "TimerQueue.h"

int createTimerfd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0) {
        LOG_FATAL("Failed in timerfd_create");
    }
    return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when) {
    int64_t microseconds = when.microSecondsSinceEpoch() -
                           Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100) {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec =
        static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(
        (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

void readTimerfd(int timerfd, Timestamp now) {
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    LOG_DEBUG("TimerQueue::handleRead() %d at %s\n", howmany, now.toString);
    if (n != sizeof howmany) {
        LOG_ERROR("TimerQueue::handleRead() reads %ld bytes instead of 8\n", n);
    }
}

/*
重新设置Timerfd的newValue，即到期时间，不修改周期性定时 时间
*/
void resetTimerfd(int timerfd, Timestamp expiration) {
    // wake up loop by timerfd_settime()
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof newValue);
    bzero(&oldValue, sizeof oldValue);
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret) {
        LOG_ERROR("timerfd_settime()\n");
    }
}

// class definition
TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_(),
      callingExpiredTimers_(false) {
    timerfdChannel_.setReadCallBack(std::bind(&TimerQueue::handleRead, this));
    // we are always reading the timerfd, we disarm it with timerfd_settime.
    timerfdChannel_.enableReading();  //开启可读事件
}

TimerQueue::~TimerQueue() {
    // channel去掉
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    // do not remove channel, since we're in EventLoop::dtor();
    //释放所有的Timer*内存
    for (const Entry& timer : timers_) {
        delete timer.second;
    }
}

/*
1.新设一个定时器Timer，设置过期回调函数，并将它加入到集合中
2.返回该定时器序号
*/
TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp when,
                             double interval) {
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

/*
取消定时器任务
1.调用cancelInLoop()删除该定时器任务，并将其添加到cancelInLoop中
*/
void TimerQueue::cancel(TimerId timerId) {
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

/*
1.调用Insert()将该计时器加入到timers_ 和 activeTimers_中
2.如果插入的最早到期的计时器，那就修改timerfd的到期时间
*/
void TimerQueue::addTimerInLoop(Timer* timer) {
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);

    if (earliestChanged) {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

/*
取消某个定时器
1.从timers_和activeTimers_从删除该定时器
2.把该定时器加入到cancelingTimers_中，防止在reset()因为有repeat标志被重新加入到计时器队列中
*/
void TimerQueue::cancelInLoop(TimerId timerId) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if (it != activeTimers_.end()) {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1);
        (void)n;
        delete it->first;  // FIXME: no delete please
        activeTimers_.erase(it);
    } else if (callingExpiredTimers_) {
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

/*
计时器可读（过期了）时的回调处理函数
    1.输出该计时器执行了几次
    2.从timers_ 和 activeTimers_中删除过期的计时器，并返回这些过期的计时器
    3.执行这些过期计时器的回调函数cb
    4.把需要重复执行的计时器重新加入到timers_ 和 activeTimers_中
*/
void TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);  //?就只输出执行了几次吗

    //从timers_ 和 activeTimers_中删除过期的计时器，并返回这些过期的计时器
    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    // safe to callback outside critical section
    for (const Entry& it : expired) {
        it.second->run();  //执行过期计时器的回调函数cb
    }
    callingExpiredTimers_ = false;
    //把需要重复执行的计时器重新加入到timers_ 和 activeTimers_中
    reset(expired, now);
}

/*
    1.从TimerList timers_（有序计时器） 和 ActiveTimerSet
   activeTimers_中删除过期的计时器 2.返回过期的计时器
*/
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
    assert(timers_.size() == activeTimers_.size());
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(
                          UINTPTR_MAX));  //#define UINTPTR_MAX   UINT64_MAX
    TimerList::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    for (const Entry& it : expired) {
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1);
        (void)n;
    }

    assert(timers_.size() == activeTimers_.size());
    return expired;
}

/*
    1.如果有repeat标志，则重新设置expiration_ = addTime(now, interval_);
    并插入到activeTimers_和timers_中
    否则就释放这个Timer*的内存
    2.保存下一个到期的计时器时间
*/
void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now) {
    Timestamp nextExpire;

    for (const Entry& it : expired) {
        ActiveTimer timer(it.second, it.second->sequence());
        if (it.second->repeat() &&
            cancelingTimers_.find(timer) == cancelingTimers_.end()) {
            // 如果有repeat标志，则重新设置expiration_ = addTime(now,
            // interval_);
            it.second->restart(now);
            //插入到activeTimers_和timers_中
            insert(it.second);
        } else {
            //否则就释放这个Timer*
            // FIXME move to a free list
            delete it.second;  // FIXME: no delete please
        }
    }

    //保存下一个到期的计时器时间
    if (!timers_.empty()) {
        nextExpire = timers_.begin()->second->expiration();
    }

    if (nextExpire.valid()) {
        resetTimerfd(timerfd_, nextExpire);
    }
}

/*
插入到timers_和activeTimers_里，返回 该定时器是否是最早到期的
*/
bool TimerQueue::insert(Timer* timer) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first) {
        earliestChanged = true;
    }
    {
        std::pair<TimerList::iterator, bool> result =
            timers_.insert(Entry(when, timer));
        assert(result.second);
        (void)result;
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result =
            activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);
        (void)result;
    }

    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}