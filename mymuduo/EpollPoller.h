#pragma once
#include <sys/epoll.h>

#include <vector>

#include "Poller.h"

class EventLoop;

/*
Poller:封装Epoll相关操作
    1.封装epoll_create
    2.封装epoll_ctl
    3.封装epoll_wait
*/
class EpollPoller : public Poller {
public:
    EpollPoller(EventLoop* loop);  // epoll_create
    ~EpollPoller() override;

    // 重写Poller接口
    void removeChannel(Channel* channel) override;  // epoll_ctl,逻辑判断+调用update()
    void updateChannel(Channel* channel) override;  // epoll_ctl,逻辑判断+调用update()
    Timestamp poll(int timeoutMs,
                   ChannelList* activeChannels) override;  // epoll_wait

private:
    const static int KInitEventListSize = 16;

    // 实际的epoll_ctl
    void update(int op, Channel* channel);
    // 填写 返回的有事件的连接
    void fillActiveChannels(int eventNums, ChannelList* activeChannels) const;
    //? Channel的revents已经在里面设置了,这里传ChannelList*可能是还需要用到activeChannels.
    int m_epollfd;  // epoll_create 创建的内核事件表的标识

    using EventList = std::vector<epoll_event>;
    EventList m_reventList; // 存储返回的发生的感兴趣事件
};