#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "CurrentThread.h"
#include "TimerId.h"
#include "Timestamp.h"
#include "noncopyable.h"

class Channel;
class Poller;
class TimerQueue;

// poll阻塞等待新事件；调用channel处理新事件
// Channel和Poller的使用者、控制者
class EventLoop : noncopyable {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    //开启/退出事件循环
    void loop();
    void quit();

    //事件回调入队列
    //在本线程中运行
    void runInLoop(Functor cb);
    //加入其他线程的队列，并唤醒所在线程
    void queueInLoop(Functor cb);
    //定时队列
    TimerId runAt(Timestamp time, TimerCallback cb);
    TimerId runAfter(double delay, TimerCallback cb);
    TimerId runEvery(double interval, TimerCallback cb);
    void cancel(TimerId timerId);
    //事件返回时间
    Timestamp pollReturnTime() const { return m_pollReturnTime; }

    // Channel处理，实际交给Poller处理
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    //线程相关
    void wakeUp();
    bool isInLoopThread() const {
        return m_threadId == CurrentThread::getTid();
    }

    void assertInLoopThread() {
        if (!isInLoopThread()) {
            abortNotInLoopThread();
        }
    }

private:
    void abortNotInLoopThread();
    using ChannelList = std::vector<Channel*>;

    int m_wakeUpFd;   //通过该变量唤醒subloop处理channel
    void handRead();  //给wakeUpChannel设置读事件回调，用作唤醒
    std::unique_ptr<Channel> m_wakeUpChannel;

    std::unique_ptr<Poller> m_poller;
    std::unique_ptr<TimerQueue> m_timerQueue;
    Timestamp m_pollReturnTime;  // Poller返回发生的channels事件的时间
    ChannelList m_activeChannels;
    void doPendingFunctors();  //执行回调

    std::atomic_bool m_callingPendingFunctors;  //标识当前loop是否在执行回调操作
    std::vector<Functor> m_pendingFunctors;  //存储loop需要执行的所有回调操作
    std::mutex m_mutex;  //保护上面vector容器中回调操作

    std::atomic_bool m_looping;  //标识当前loop执行状态
    std::atomic_bool m_quit;     //标识当前loop退出状态
    const pid_t m_threadId;      //标识当前loop的线程ID
};