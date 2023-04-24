#include "EpollPoller.h"

#include <asm-generic/errno-base.h>
#include <strings.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>

#include "Channel.h"
#include "Logger.h"
#include "Poller.h"
#include "Timestamp.h"

const int kNew = -1;     // channel尚未添加到Poller中(即有没有被监听)
const int kAdded = 1;    // channel已添加到Poller中
const int kDeleted = 2;  // channel已从Poller中删除

EpollPoller::EpollPoller(EventLoop* loop)
    : Poller(loop), m_epollfd(epoll_create1(EPOLL_CLOEXEC)), m_reventList(KInitEventListSize) {
    if (m_epollfd < 0) {
        LOG_FATAL("epoll_create %d\n", errno);
    }
}

EpollPoller::~EpollPoller() {
    ::close(m_epollfd);  // 这样使用::表示是全局变量或全局函数，起到修饰的作用
}

void EpollPoller::removeChannel(Channel* channel) {
    const int fd = channel->get_fd();
    LOG_INFO("fun=%s fd=%d\n", __FUNCTION__, fd);
    m_channel_map.erase(fd);
    if (channel->get_index() == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EpollPoller::updateChannel(Channel* channel) {
    const int index = channel->get_index();
    const int fd = channel->get_fd();
    LOG_INFO("func=%s fd=%d events=%d index=%d\n", __FUNCTION__, fd, channel->get_events(), index);
    if (index == kNew || index == kDeleted) {  // 可以新增
        if (index == kNew) {
            m_channel_map[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {  // KAdded,可以删除或修改
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

Timestamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    LOG_DEBUG("fun=%s fd total count:%lu\n", __FUNCTION__, m_channel_map.size());
    int eventNums = epoll_wait(m_epollfd, &*m_reventList.begin(),
                               static_cast<int>(m_reventList.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if (eventNums > 0) {
        LOG_DEBUG("%d events happed\n", eventNums);
        fillActiveChannels(eventNums, activeChannels);

        // 全部都用上了，可能存在更多的事件，那么就需要扩容
        if (eventNums == m_reventList.size()) {
            m_reventList.resize(m_reventList.size() * 2);
        }
    } else if (eventNums == 0) {
        // 超时了
        LOG_DEBUG("%s timeout \n", __FUNCTION__);
    } else {
        // 真出错了,且不是EINTR错误
        if (saveErrno != EINTR) {
            errno = saveErrno;
            LOG_ERROR("%s error %d\n", __FUNCTION__, errno);
        }
    }
    return now;
}

void EpollPoller::update(int op, Channel* channel) {
    epoll_event events;
    bzero(&events, sizeof(epoll_event));
    events.events = channel->get_events();
    events.data.ptr = channel;
    int fd = channel->get_fd();
    if (::epoll_ctl(m_epollfd, op, fd, &events) < 0) {
        if (op == EPOLL_CTL_DEL) {
            LOG_ERROR("epoll_ctl del errno %d\n", errno);
        } else {
            LOG_FATAL("epoll_ctl add/mod errno %d\n", errno);
        }
    }
}

void EpollPoller::fillActiveChannels(int eventNums, ChannelList* activeChannels) const {
    for (int i = 0; i < eventNums; i++) {
        Channel* channel = static_cast<Channel*>(m_reventList[i].data.ptr);
        channel->set_revents(m_reventList[i].events);
        activeChannels->push_back(channel);
    }
}
