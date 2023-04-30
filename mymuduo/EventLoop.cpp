#include "EventLoop.h"
#include "Channel.h"
#include "CurrentThread.h"
#include "Logger.h"
#include "Poller.h"

#include <sys/eventfd.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

//防止一个线程创建多个EventLoop
thread_local EventLoop* t_loopInthisThread = nullptr;
//定义默认的Poller IO阻塞超时时间
const int kPollTimeMs = 10000;

//创建wakeUpFd，用来notify唤醒subReactor处理新来的channel
int createEventfd() {
    int eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (eventfd < 0) {
        LOG_FATAL("create eventfd errno %d\n", errno);
    }
    return eventfd;
}
EventLoop::EventLoop()
    : m_wakeUpFd(createEventfd()),
      //将该m_wakeUpfd与Channel绑定在一起,以后m_wakeUpchannel就代表着m_wakeUpFd的事件集合
      m_wakeUpChannel(new Channel(this, m_wakeUpFd)),
      m_poller(Poller::newDefaultPoller(this)),
      m_callingPendingFunctors(false),
      m_looping(false),
      m_quit(false),
      m_threadId(CurrentThread::getTid()) {
    if (t_loopInthisThread) {
        LOG_FATAL("Another EventLoop existed in this thread\n");
    } else {
        t_loopInthisThread = this;
    }
    LOG_DEBUG("EventLoop %p created in thread %lu\n", this, m_threadId);
    //设置wakeUpChannel的读回调函数
    m_wakeUpChannel->setReadCallBack(std::bind(&EventLoop::handRead, this));
    //每一个EventLoop都将监听wakeUpChannel的的EPOLLIN读事件
    m_wakeUpChannel->enableReading();
}

EventLoop::~EventLoop() {
    //该Channel不再接收事件
    m_wakeUpChannel->disableAll();
    m_wakeUpChannel->remove();
    //关闭这一个fd
    ::close(m_wakeUpFd);
    //销毁本线程loop
    t_loopInthisThread = nullptr;
}

//开启事件循环
void EventLoop::loop() {
    m_looping = true;
    m_quit = false;
    LOG_INFO("EventLoop %p start looping\n", this);
    while (!m_quit) {
        m_activeChannels.clear();
        m_pollReturnTime = m_poller->poll(kPollTimeMs, &m_activeChannels);
        for (Channel* channel : m_activeChannels) {
            channel->handleEvent(m_pollReturnTime);
        }
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping\n", this);
    m_looping = false;
}

//退出事件循环
void EventLoop::quit() {
    m_quit = true;
    if (!isInLoopThread()) {
        wakeUp();
    }
}

//在当前loop中执行cb
void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_pendingFunctors.push_back(cb);
    }
    if (!isInLoopThread() || m_callingPendingFunctors) {
        wakeUp();
    }
}

void EventLoop::updateChannel(Channel* channel) {
    m_poller->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    m_poller->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
    return m_poller->hasChannel(channel);
}

//唤醒wake所在的线程
void EventLoop::wakeUp() {
    uint64_t one = 1;
    ssize_t ret = write(m_wakeUpFd, &one, sizeof(one));
    if (ret != sizeof(one)) {
        LOG_ERROR("EventLoop::wakeup writes %lu bytes instead of 8 \n", ret);
    }
}

void EventLoop::handRead() {
    uint64_t one = 1;
    ssize_t ret = read(m_wakeUpFd, &one, sizeof(one));
    if (ret != sizeof(one)) {
        LOG_ERROR("EventLoop::handRead reads %lu bytes instead of 8 \n", ret);
    }
}

//执行回调
void EventLoop::doPendingFunctors() {
    m_callingPendingFunctors = true;
    std::vector<Functor> functors;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        functors.swap(m_pendingFunctors);
    }
    for (const Functor& functor : functors) {
        functor();
    }
    m_callingPendingFunctors = false;
}
