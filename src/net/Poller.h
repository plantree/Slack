/*
 * @Author: py.wang 
 * @Date: 2019-05-18 08:47:22 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-21 08:47:50
 */

#ifndef NET_POLLER_H
#define NET_POLLER_H

#include "src/base/Timestamp.h"
#include "src/net/EventLoop.h"

#include <vector>
#include <map>

namespace slack
{

namespace net
{
// 前向声明
class Channel;

// Base class for IO multiplexing
// but it doesn't own the Channel objects
// 不负责管理Channel生存期
// Abstract class，负责定义高层接口
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller();  // 存在继承，所以是虚析构

    // Polls the IO events, must be called in the loop thead
    // pure virtual function
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;

    // change the interested IO events, must be called in the loop thread
    virtual void updateChannel(Channel *channel) = 0;

    // remove the channel, when it destructs, must be called in the loop thread
    virtual void removeChannel(Channel *channel) = 0;

    virtual bool hasChannel(Channel *channel) const;
    
    static Poller *newDefaultPoller(EventLoop *loop);

    void assertInLoopThread() const
    {
        ownerLoop_->assertInLoopThread();
    }

protected:
    // 便于查找
    using ChannelMap = std::map<int, Channel *>;
    ChannelMap channels_;
private:
    EventLoop *ownerLoop_;  // 归属EventLoop
};

}   // namespace net

}   // namespace slack

#endif  // NET_POLLER_H
