/*
 * @Author: py.wang 
 * @Date: 2019-05-19 08:27:39 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-21 08:49:10
 */

#ifndef NET_POLLER_EPOLLPOLLER_H
#define NET_POLLER_EPOLLPOLLER_H

#include "src/net/Poller.h"

#include <vector>

// 前向声明
struct epoll_event;

namespace slack
{

namespace net
{

// IO multiplexing with epoll(4)
// 继承抽象类，要实现接口
class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop *loop);
    virtual ~EpollPoller() override;
    // 抽象基类接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    virtual void updateChannel(Channel *channel) override;
    virtual void removeChannel(Channel *channel) override;

private:
    // 初始Event大小
    static const int kInitEventListSize = 16;

    static const char *operationToString(int op);

    // 辅助函数
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    void update(int operation, Channel *channel);

    using EventList = std::vector<struct epoll_event>;

    int epollfd_;
    // 事件列表
    EventList events_;
};


}   // namespace net

}   // namespace slack

#endif // NET_POLLER_EPOLLPOLLER_H