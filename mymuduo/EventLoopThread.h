#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

#include "Thread.h"
#include "noncopyable.h"

class EventLoop;

//创建新的线程，并创建属于它的新事件循环，返回该事件循环指针，并开启该事件循环
//形成one loop per thread的结构，Thread和EventLoop的使用者、控制者
class EventLoopThread : private noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& initCallback,
                    const std::string& name);
    ~EventLoopThread();

    EventLoop* startLoop();

private:
    void threadFunc();  //线程需要执行的函数，作为线程的初始化变量

	//外部传进m_threadInitCallback，用来在线程初始化时做的事情
	//来源：外部传入->TcpServer::m_threadInitCallback->TcpServer::start()
	//->m_threadPool->start(m_threadInitCallback)->EventLoopThreadPool::start(const ThreadInitCallback& cb)
	//->EventLoopThread* t = new EventLoopThread(cb, buf);
	//->threadFunc()-> m_threadInitCallback(&loop);  
    ThreadInitCallback m_threadInitCallback;
    bool m_exiting;
    EventLoop* m_loop;
    Thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};