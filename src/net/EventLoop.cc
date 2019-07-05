/*
 * @Author: py.wang 
 * @Date: 2019-05-18 07:52:55 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-01 08:02:56
 */

#include "src/base/Mutex.h"
#include "src/log/Logging.h"
#include "src/net/EventLoop.h"
#include "src/net/Poller.h"
#include "src/net/Channel.h"
#include "src/net/SocketsOps.h"
#include "src/net/TimerQueue.h"

#include <assert.h>
// 创建事件对象，用于等待、通知
#include <sys/eventfd.h>
#include <sys/signal.h>
#include <unistd.h>

using namespace slack;
using namespace slack::net;

// 匿名空间
namespace 
{
// 当前线程EventLoop对象指针
// 线程局部存储
__thread EventLoop *t_loopInThisThread = nullptr;

// 超时时间
const int kPollTimeMs = 10000;

// 用于事件通知
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_SYSERR << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
// 忽略SIGPIPE
class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
        LOG_TRACE << "Ignore SIGPIPE";
    }
};
#pragma GCC diagnostic error "-Wold-style-cast"

IgnoreSigPipe initObj;

}   // namespace 

// static function
EventLoop *EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctos_(false),
    iteration_(0),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),    // default epoll
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()), // 唤醒事件
    wakeupChannel_(new Channel(this, wakeupFd_)),
    currentActiveChannel_(nullptr)
{
    // 如果当前线程已经创建了EventLoop对象，终止（LOG_FATAL）
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
            << " exists in this thread " << threadId_;
    }
    else 
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // we always reading the wakeupfd
    wakeupChannel_->enableReading();
    
    LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
}

EventLoop::~EventLoop()
{
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
        << " destructors in thread " << CurrentThread::tid();
    // remove channel，其他的会自动析构
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// 事件循环，不能跨越线程调用
// 只能在创建对象的线程中
void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();

    looping_ = true;
    quit_ = false;
    LOG_TRACE << "EventLoop " << this << " start looping";

    // 无限循环，除非被中止
    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        ++iteration_;
        if (Logger::logLevel() <= Logger::TRACE)
        {
            printActiveChannels();
        }
        // TODO sort channel by priority
        eventHandling_ = true;
        for (Channel *channel : activeChannels_)
        {
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = nullptr;
        eventHandling_ = false;

        // 处理挂起的的回调函数
        doPendingFunctors();
    }

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

// 该函数可以跨线程调用
void EventLoop::quit()
{
    quit_ = true;
    // 非本线程调用，要唤醒
    if (!isInLoopThread())
    {
        wakeup();
    }
}

// 在IO线程中执行回调函数，可以跨线程调用
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())
    {
        // 当前IO线程，同步调用
        cb();
    }
    else 
    {
        // 其他线程调用则放入队列
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    // 多线程并发访问，要用锁
    {
        MutexLockGuard lock(mutex_);
        pendingFunctos_.push_back(std::move(cb));
    }

    // 调用queueInLoop的线程不是IO线程需要唤醒
    // 或者调用queueInLoop的线程是IO线程，并且此时正在处理pending functor需要唤醒
    // 只有IO线程的事件回调中queueInLoop才不需要唤醒
    if (!isInLoopThread() || callingPendingFunctos_)
    {
        wakeup();
    }
}

size_t EventLoop::queueSize() const 
{
    MutexLockGuard lock(mutex_);
    return pendingFunctos_.size();
}

TimerId EventLoop::runAt(Timestamp time, TimerCallback cb)
{ 
    return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
    return timerQueue_->cancel(timerId);
}

void EventLoop::updateChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();

    // 实际操作转为Poller执行
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    // 正在处理事件
    if (eventHandling_)
    {
        // 要么正在处理，要么不在activeChannels_列表
        assert(currentActiveChannel_ == channel || 
            std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "eventLoop::abortNotInLoopThread - EventLoop " << this 
        << " was created in threadId_ = " << threadId_
        << ", current thread id = " << CurrentThread::tid();
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    // 写入，唤醒
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

// 响应读事件
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

// 执行挂起回调
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctos_ = true;

    {
        MutexLockGuard lock(mutex_);
        // 清空回调队列，交换到局部变量，缩小临界区
        functors.swap(pendingFunctos_);
    }

    for (const Functor &functor : functors)
    {
        functor();
    }

    callingPendingFunctos_ = false;
}

void EventLoop::printActiveChannels() const 
{
    for (const Channel *channel : activeChannels_)
    {
        LOG_TRACE << "{" << channel->reventsToString() << "}";
    }
}