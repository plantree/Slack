/*
 * @Author: py.wang 
 * @Date: 2019-05-18 08:55:29 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-21 08:47:08
 */

#include "src/net/Poller.h"
#include "src/net/Channel.h"

using namespace slack;
using namespace slack::net;

/**
 * just an abstract class
 */
Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop)
{
}

// channels_(std::map)会自动析构
Poller::~Poller() = default;

bool Poller::hasChannel(Channel *channel) const
{
    assertInLoopThread();
    ChannelMap::const_iterator it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}