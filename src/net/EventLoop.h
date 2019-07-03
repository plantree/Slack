/*
 * @Author: py.wang 
 * @Date: 2019-05-17 09:35:31 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-01 09:31:25
 */

#ifndef NET_EVENTLOOP_H
#define NET_EVENTLOOP_H

#include "src/base/Mutex.h"
#include "src/base/CurrentThread.h"
#include "src/base/Timestamp.h"
#include "src/net/Callbacks.h"
#include "src/net/TimerId.h"

#include <atomic>
#include <memory>
#include <functional>
#include <vector>

namespace slack
{

namespace net
{

// 前向声明
class Channel;
class Poller;
class TimerQueue;

// Reactor, at most one per thread
// This is an interface class, so don't expose too much details
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // loops forever. Must be called in the same thread as creation of the object
    void loop();

    // quits loop, not 100% thread safe, better to call through shared_ptr<EventLoop>
    void quit();

    // Time when poll returns, ususlly means data arrival
    Timestamp pollReturnTime() const 
    {
        return pollReturnTime_;
    }

    int64_t iteration() const 
    {
        return iteration_;
    }

    // Runs callback immediately in the loop thread
    // It wakes up the loop, and run the cb
    // If in the same loop thread, cb is run within the function
    // Safe to call from other threads
    // 立马执行
    void runInLoop(Functor cb);
    // Queues callback in the loop thread
    // Runs after finish polling
    // Safe to call from other threads
    // 排队执行
    void queueInLoop(Functor cb);

    size_t queueSize() const;

    // timers

    // runs callback at 'time'
    // safe to call from other threads
    TimerId runAt(Timestamp time, TimerCallback cb);

    // runs callback after delay seconds
    // safe to call from other threads
    TimerId runAfter(double delay, TimerCallback cb);

    // runs callback every intarval seconds
    // safe to call from other threads
    TimerId runEvery(double interval, TimerCallback cb);

    // cancels the timer
    // safe to call from other threads
    void cancel(TimerId timerId);

    // internal usage
    void wakeup();
    void updateChannel(Channel *channel);   // 从Poller中添加或者更新通道
    void removeChannel(Channel *channel);   // 从Poller中删除通道
    bool hasChannel(Channel *channel);

    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }

    // one loop per thread
    bool isInLoopThread() const 
    {
        return threadId_ == CurrentThread::tid();
    }

    // 事件正在处理
    bool eventHandling() const 
    {
        return eventHandling_;
    }

    static EventLoop *getEventLoopOfCurrentThread();
private:
    void abortNotInLoopThread();
    void handleRead();  // wake up
    void doPendingFunctors();

    void printActiveChannels() const;   // DEBUG

    using ChannelList = std::vector<Channel *>;

    bool looping_;  // atomic
    std::atomic<bool> quit_;     // atomic
    bool eventHandling_;    // atomic
    bool callingPendingFunctos_;    // atomic

    int64_t iteration_;
    const pid_t threadId_;  // local thread ID
    Timestamp pollReturnTime_;

    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;    // 定时器队列

    int wakeupFd_;  // 用于eventfd
    
    // unlike in TimeQueue, whichi is an internal class
    // we don't expose channel to client
    std::unique_ptr<Channel> wakeupChannel_;    // 绑定wakeupFd_, 纳入poller_管理

    ChannelList activeChannels_;    // Poller中返回的活跃通道
    Channel *currentActiveChannel_; // 正在处理的活跃通道
    
    mutable MutexLock mutex_;
    std::vector<Functor> pendingFunctos_;   // guardedBy mutex_;
};

}   // namespace net

}   // namespace slack

#endif  // NET_EVENTLOOP_H