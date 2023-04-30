#include "EventLoopThread.h"
#include "EventLoop.h"

#include <functional>
#include <mutex>

EventLoopThread::EventLoopThread(const ThreadInitCallback& initCallback,
                                 const std::string& name)
    : m_threadInitCallback(initCallback),
      m_exiting(false),
      m_loop(nullptr),
      m_thread(std::bind(&EventLoopThread::threadFunc, this), name),
      m_mutex(),
      m_cond() {}

//线程退出
EventLoopThread::~EventLoopThread() {
    m_exiting = true;
    if (m_loop != nullptr) {
        m_loop->quit();   //如果事件循环存在，那就退出
        m_thread.join();  //等待该线程的子线程退出
    }
}

//以下这两个函数就是实现 one loop per thread 的部分

//启动新的事件循环，并返回该事件循环指针
EventLoop* EventLoopThread::startLoop() {
    m_thread.start();  //启动一个新的线程
    //在自动调用线程创建对应的事件循环后，取得该事件循环的指针 （所以threadFunc是个private函数，是供startLoop调用的）
    EventLoop* tmp_loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_loop == nullptr) {
            m_cond.wait(lock);
        }
        tmp_loop = m_loop;
    }
    return tmp_loop;
}

//线程需要执行的函数，作为线程的初始化变量
void EventLoopThread::threadFunc() {
    EventLoop loop;  //one loop per thread
    if (m_threadInitCallback) {
        m_threadInitCallback(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_loop = &loop;
        m_cond.notify_one();
    }
    loop.loop();  // EventLoop loop  => Poller.poll
    //这里就是事件循环/线程返回了（结束了），那么就销毁该事件循环
    std::unique_lock<std::mutex> lock(m_mutex);
    m_loop = nullptr;
}
