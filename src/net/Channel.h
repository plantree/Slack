/*
 * @Author: py.wang 
 * @Date: 2019-05-18 09:20:26 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-20 09:19:24
 */

#ifndef NET_CHANNEL_H
#define NET_CHANNEL_H

#include "src/base/noncopyable.h"
#include "src/base/Timestamp.h"

#include <functional>
#include <memory>

namespace slack
{

namespace net
{

class EventLoop;

// A selectbale IO channel
// This class doesn't own the file descriptor.
// who opened it, who should close it
// The file descriptor could be a socket, 
// an eventfd, a timerfd, or a signalfd
// 对于事件的统一封装
class Channel : noncopyable
{
public:
    // 回调函数
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);
    // 设置读回调
    void setReadCallback(ReadEventCallback cb)
    {
        readCallback_ = std::move(cb);
    }
    // 设置写回调
    void setWriteCallback(EventCallback cb)
    {
        writeCallback_ = std::move(cb);
    }
    // 设置关闭时回调
    void setCloseCallback(EventCallback cb)
    {
        closeCallback_ = std::move(cb);
    }
    // 设置错误回调
    void setErrorCallback(EventCallback cb)
    {
        errorCallback_ = std::move(cb);
    }

    // FIXME
    // Tie this channel to the owner object managed by shared_ptr,
    // prevent the owner object being destroyed in handleEvent
    // 主要是为了控制生存期，所属对象的生存期比Channel短
    // 会造成一些意外情况
    void tie(const std::shared_ptr<void> &);

    int fd() const 
    {
        return fd_;
    }
    int events() const 
    {
        return events_;
    }
    void set_revents(int revt)
    {
        revents_ = revt;    // used by pollers
    }
    int revents() const 
    {
        return revents_;
    }
    bool isNoneEvent() const 
    {
        return events_ == kNoneEvent;
    }

    void enableReading()
    {
        events_ |= kReadEvent;
        update();   // calling Poller
    }
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disbaleWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }
    bool isReading() const 
    {
        return events_ & kReadEvent;
    }
    bool isWriting() const 
    {
        return events_ & kWriteEvent;
    }

    // for poller
    int index() const 
    {
        return index_;
    }
    void set_index(int idx)
    {
        index_ = idx;
    }

    // for debug
    string reventsToString() const;
    string eventsToString() const;

    // 挂起
    void doNotLogHup()
    {
        logHup_ = false;
    }

    // 归属EventLoop(有且只有一个)
    EventLoop *ownerLoop()
    {
        return loop_;
    }
    void remove();

private:
    static string eventsToString(int fd, int event);

    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;    // 无事件
    static const int kReadEvent;    // 读事件
    static const int kWriteEvent;   // 写事件

    EventLoop *loop_;   // channel所属EventLoop
    const int fd_;      // 文件描述符
    int events_;        // 关注的事件
    int revents_;       // poller返回的事件
    int index_;         // 在poller数组中的状态
    bool logHup_;       // for POLLHUP

    std::weak_ptr<void> tie_;
    bool tied_;
    bool eventHandling_;    // 是否处于处理事件中
    bool addedToLoop_;
    
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};

}   // namespace net

}   // namespace slack

#endif // NET_CHANNEL_H