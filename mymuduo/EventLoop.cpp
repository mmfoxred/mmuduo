#include "EventLoop.h"
#include "Channel.h"
#include "CurrentThread.h"
#include "Logger.h"
#include "Poller.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <cstddef>
#include <functional>

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
