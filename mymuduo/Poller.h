#pragma once
#include <unordered_map>
#include <vector>

#include "Channel.h"
#include "Timestamp.h"
class EventLoop;
class Channel;

/*
Poller:封装Epoll相关操作
    1.封装epoll_create
    2.封装epoll_ctl
    3.封装epoll_wait
*/
class Poller {
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);  // epoll_create
    virtual ~Poller() = default;

    // 提供具体实例 单例模式
    static Poller* newDefaultPoller(EventLoop* loop);
    // 判断某channel是否注册到了Poller中
    bool hasChannel(Channel* channel) const;
    // 给IO复用保留统一接口 epoll_ctl  epoll_wait
    virtual void removeChannel(Channel* channel) = 0;  // epoll_ctl
    virtual void updateChannel(Channel* channel) = 0;  // epoll_ctl
    virtual Timestamp poll(int timeoutMs,
                           ChannelList* activeChannels) = 0;  // epoll_wait

protected:
    //<sockfd:Channel> dict
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap m_channel_map;

private:
    EventLoop* m_owner_loop;
};