/*
 * @Author: py.wang 
 * @Date: 2019-05-19 09:29:59 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-21 08:57:25
 */

#include "src/net/poller/EpollPoller.h"
#include "src/net/Channel.h"
#include "src/log/Logging.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

using namespace slack;
using namespace slack::net;

// On linux, the constants of poll(2) and epoll(4)
// are expected to be the same
static_assert(EPOLLIN == POLLIN,        "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI,      "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT,      "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP,  "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR,      "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP,      "epoll uses same flag values as poll");

// 匿名命名空间，文件内部使用
namespace 
{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),   // 没必要非阻塞
    events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_SYSFATAL << "EpollPoller::EpollPoller";
    }
};

EpollPoller::~EpollPoller()
{
    ::close(epollfd_);
}

// 轮询出到达事件
Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    LOG_TRACE << "fd total count " << channels_.size();
    // 阻塞
    int numEvents = ::epoll_wait(epollfd_,
                            events_.data(),
                            static_cast<int>(events_.size()),
                            timeoutMs);
    
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    // 有事件到达
    if (numEvents > 0)
    {
        LOG_TRACE << numEvents << " events happeded";
        fillActiveChannels(numEvents, activeChannels);
        // events_数组扩容
        if (implicit_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_TRACE << " nothing happeded";
    }
    else 
    {
        // error happens ,log uncommon ones
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_SYSERR << "EpollPoller::poll()";
        }
    }
    return now;
}

void EpollPoller::fillActiveChannels(int numEvents, 
                                    ChannelList *activeChannels) const
{
    assert(implicit_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
    #ifndef NDEBUG
        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
    #endif
        // 设置事件到达
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EpollPoller::updateChannel(Channel *channel)
{
    // 必须在IO线程
    Poller::assertInLoopThread();
    const int index = channel->index();
    // 文件描述符->事件->状态
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events()
            << " index = " << index;
    // 新增、删除
    if (index == kNew || index == kDeleted)
    {
        // a new one, add with EPOLL_CTL_ADD
        int fd = channel->fd();
        if (index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            // 放入map
            channels_[fd] = channel;
        }
        else 
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        // 更新状态和事件
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else 
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd(); (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        // 没有监听事件
        if (channel->isNoneEvent())
        {   
            // 删除事件
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else 
        {
            // 修改事件
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::removeChannel(Channel *channel)
{
    // 必须在IO线程内
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG_TRACE << "fd = " << fd;

    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    // 没有监听事件才能删除
    assert(channel->isNoneEvent());

    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    // 从map中移除
    size_t n = channels_.erase(fd); (void) n;
    assert(n == 1);

    if (index == kAdded)
    {
        // 更新事件，不再关注
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EpollPoller::update(int operation, Channel *channel)
{
    struct epoll_event event;
    memZero(&event, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    LOG_TRACE << "epoll_ctl op = " << operationToString(operation);
    
    // ERROR
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_SYSERR << "epoll_ctl op = " << operationToString(operation) << " fd = " << fd;
        }
        else 
        {
            LOG_SYSFATAL << "epoll_ctl op = " << operationToString(operation) << " fd = " << fd;
        }
    }
}

const char *EpollPoller::operationToString(int operation)
{
    switch (operation)
    {
    case EPOLL_CTL_ADD:
        return "ADD";
    case EPOLL_CTL_DEL:
        return "DEL";
    case EPOLL_CTL_MOD:
        return "MOD";
    default:
        assert(false && "ERROR op");
        return "Unknown Operation";
    }
}