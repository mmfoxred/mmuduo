#include "Thread.h"
#include <semaphore.h>
#include <atomic>
#include <cstdio>
#include <memory>
#include <thread>
#include <utility>
#include "CurrentThread.h"

std::atomic_int Thread::m_numCount(0);

Thread::Thread(ThreadFunc func, const std::string& name)
    : m_started(false),
      m_joined(false),
      m_tid(0),
      m_name(name),
      m_funcs(std::move(func)) {
    setDefaultName();
}

Thread::~Thread() {
    if (m_started && !m_joined) {
        m_thread->detach();
    }
}

//启动线程
void Thread::start() {
    m_started = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    //新键线程来执行指定的funcs
    m_thread = std::shared_ptr<std::thread>(new std::thread([&]() {
        m_tid = CurrentThread::getTid();
        sem_post(&sem);
        m_funcs();
    }));
    sem_wait(&sem);
}

void Thread::join() {
    m_joined = true;
    m_thread->join();
}

void Thread::setDefaultName() {
    int num = ++m_numCount;
    if (m_name.empty()) {
        char bufName[32] = {0};
        snprintf(bufName, sizeof(bufName), "Thread %d", num);
        m_name = bufName;
    }
}
