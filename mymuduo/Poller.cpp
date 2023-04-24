#include "Poller.h"
Poller::Poller(EventLoop* loop) : m_owner_loop(loop) {}

bool Poller::hasChannel(Channel* channel) const {
    ChannelMap::const_iterator it = m_channel_map.find(channel->get_fd());
    return it != m_channel_map.end() && it->second == channel;
}
