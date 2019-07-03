/*
 * @Author: py.wang 
 * @Date: 2019-05-19 08:50:34 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-29 09:08:03
 */

#include "src/log/Logging.h"
#include "src/net/Channel.h"
#include "src/net/EventLoop.h"

#include <sstream>

#include <poll.h>

using namespace slack;
using namespace slack::net;

// 静态成员变量定义
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;   // 有一些额外的条件，如紧急数据
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop),
    fd_(fd),
    events_(0),
    revents_(0),
    index_(-1), // kNew
    logHup_(true),
    tied_(false),
    eventHandling_(false),
    addedToLoop_(false)
{
}

Channel::~Channel()
{
    assert(!eventHandling_);
    assert(!addedToLoop_);
    if (loop_->isInLoopThread())
    {
        assert(!loop_->hasChannel(this));
    }
}

void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;     // 弱引用
    tied_ = true;
}

void Channel::update()
{
    addedToLoop_ = true;
    loop_->updateChannel(this); // calling poller
}

// 调用该函数前确保disbaleAll
void Channel::remove()
{
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

// FIXME
// 确保对象还活着，否则事件将不会被处理
void Channel::handleEvent(Timestamp receiveTime)
{
    std::shared_ptr<void> guard;
    // 弱引用
    if (tied_)
    {
        guard = tie_.lock();
        if (guard)
        {
            // 保证对象还活着
            handleEventWithGuard(receiveTime);
        }
    }
    else 
    {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    // 正在处理事件
    eventHandling_ = true;
    LOG_TRACE << reventsToString();
    // the peer closed its end of the channel
    // 挂起且不可读
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if (logHup_)
        {
            LOG_WARN << "fd = " << fd_ << " Channle::handle_event() POLLHUP";
        }
        if (closeCallback_)
        {
            closeCallback_();
        }
    }

    if (revents_ & POLLNVAL)    // invalid request
    {
        LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
    }

    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }

    // 读事件
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }

    // 写事件
    if (revents_ & POLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
    eventHandling_ = false;
}

string Channel::reventsToString() const 
{
    return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const 
{
    return eventsToString(fd_, events_);
}

string Channel::eventsToString(int fd, int event) 
{
    std::ostringstream oss;
    oss << fd << ": ";
    if (event & POLLIN)
    {
        oss << "IN ";
    }
    if (event & POLLPRI)
    {
        oss << "PRI ";
    }
    if (event & POLLOUT)
    {
        oss << "OUT ";
    }
    if (event & POLLRDHUP)
    {
        oss << "RDHUP ";
    }
    if (event & POLLERR)
    {
        oss << "ERR ";
    }
    if (event & POLLNVAL)
    {
        oss << "NVAL ";
    }
    return oss.str();
}


