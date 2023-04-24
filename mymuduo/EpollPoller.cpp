#include "EpollPoller.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <cerrno>
#include "Channel.h"
#include "Logger.h"
#include "Poller.h"
#include "Timestamp.h"

//channel尚未添加到Poller中
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop* loop)
    : Poller(loop),
      m_epollfd(epoll_create1(EPOLL_CLOEXEC)),
      m_reventList(KInitEventListSize) {
    if (m_epollfd < 0) {
        LOG_FATAL("epoll_create %d\n", errno);
    }
}

EpollPoller::~EpollPoller() {
    ::close(m_epollfd);  //这样使用::表示是全局变量或全局函数，起到修饰的作用
}

Timestamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    LOG_DEBUG("fun=%s fd total count:%lu\n", __FUNCTION__,
              m_channel_map.size());
    int eventNums =
        epoll_wait(m_epollfd, &*m_reventList.begin(),
                   static_cast<int>(m_reventList.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if (eventNums > 0) {
        LOG_DEBUG("%d events happed\n", eventNums);
        fillActiveChannels(eventNums, activeChannels);

        //全部都用上了，可能存在更多的事件，那么就需要扩容
        if (eventNums == m_reventList.size()) {
            m_reventList.resize(m_reventList.size() * 2);
        }
    }
    return now;
}

void EpollPoller::fillActiveChannels(int eventNums,
                                     ChannelList* activeChannels) const {
    for (int i = 0; i < eventNums; i++) {
        Channel* channel = static_cast<Channel*>(m_reventList[i].data.ptr);
        channel->set_revents(m_reventList[i].events);
        activeChannels->push_back(channel);
    }
}
