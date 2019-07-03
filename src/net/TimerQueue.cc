/*
 * @Author: py.wang 
 * @Date: 2019-05-22 08:20:25 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-23 09:37:42
 */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include "src/log/Logging.h"
#include "src/net/TimerQueue.h"
#include "src/net/Timer.h"
#include "src/net/TimerId.h"
#include "src/net/EventLoop.h"

#include <sys/timerfd.h>
#include <unistd.h>
#include <ctype.h>

namespace slack
{

namespace net
{

namespace detail
{
// 创建定时器描述符
// timerfd，纳入poller体系
int createTimerfd()
{
    // CLOCK_MONOTONIC 任何进程都无法更改的单调递增的时钟
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        LOG_SYSFATAL << "Failed in timerfd_create";
    }
    return timerfd;
}

// 计算超时时刻与当前时间的时间差
struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    // FIXME
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(microseconds % Timestamp::kMicroSecondsPerSecond) * 1000;
    return ts;
}

// 清除定时器，避免一直触发
// 有读事件说明定时器到时
void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    // 有读事件，就拿出来
    ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
    LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if (n != sizeof(howmany))
    {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}

// 重置定时器的超时时间
void resetTimerfd(int timerfd, Timestamp expiration)
{
    // wake up loop by timerfd_settime()
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memZero(&newValue, sizeof newValue);
    memZero(&oldValue, sizeof oldValue);

    // 一个相对时间，it_interval=0说明只激发一次
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        LOG_SYSERR << "timerfd_settime()";
    }
}

}   // namespace detail

}   // namespace net

}   // namespace muduo

using namespace slack;
using namespace slack::net;
using namespace slack::net::detail;

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
    timerfd_(createTimerfd()),
    timerfdChannel_(loop, timerfd_),
    timers_(),
    callingExpiredTimers_(false)
{
    // 处理timerfd_读事件，也就是有时间到期
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    // we are always reading the timerfd, we disarm it with timerfd_settime
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    // FIXME
    // 先disbaleAll，再移除Poller关注
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);

    // do not remove channel, since we're in EventLoop::dtor()
    // 删除TimerQueue中的所有定时器
    for (const Entry &timer : timers_)
    {
        // delete timer
        delete timer.second;
    }
}

TimerId TimerQueue::addTimer(TimerCallback cb,
                            Timestamp when,
                            double interval)
{
    Timer *timer = new Timer(std::move(cb), when, interval);
    // 必须在IO线程执行
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
    // the same
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(Timer *timer)
{
    loop_->assertInLoopThread();
    // 插入一个定时器(到红黑树)，有可能会使得最早到期的定时器发生改变
    bool earliestChanged = insert(timer);

    if (earliestChanged)
    {
        // 重置定时器的超时时刻
        resetTimerfd(timerfd_, timer->expriation());
    }
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    
    // 查找该定时器
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    // 找到并删除
    if (it != activeTimers_.end())
    {
        // 删除两个地方，timers_和activeTimers_
        size_t n = timers_.erase(Entry(it->first->expriation(), it->first));
        assert(n == 1); (void)n;
        // delete *timer
        delete it->first;   // FIXME: 如果用了unique_ptr就不用手动删除了
        activeTimers_.erase(it);
    }
    else if (callingExpiredTimers_)
    {
        // 已经过期，并且正在调用回调函数的定时器
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

// 处理读事件，回调给Channel
void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);     // 清除该事件，避免一直触发

    // 获取超时定时器列表
    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    // 保证取消队列为空
    cancelingTimers_.clear();
    // safe to callback outside critical section
    for (const Entry &it : expired)
    {
        // 定时器运行回调
        it.second->run();
    }

    callingExpiredTimers_ = false;

    // 如果不是一次性定时器，则需要重启
    reset(expired, now);
}

// RVO返回值优化
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    assert(timers_.size() == activeTimers_.size());
    std::vector<Entry> expired;
    // FIXME，参考点(极大值)
    Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX));

    // lower_bound的含义是返回第一个>=sentry的元素的iterator
    // 即*end >= sentry，从而end->first > now
    TimerList::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);
    // 到期的定时器插入(不包含end)
    std::copy(timers_.begin(), end, std::back_inserter(expired));
    
    // 从timers_中移除
    timers_.erase(timers_.begin(), end);
    // 从activeTimers_中移除
    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1); (void)n;
    }

    assert(timers_.size() == activeTimers_.size());
    return expired;
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
{
    Timestamp nextExpried;

    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        // 重启重复定时器,如果定时器不在取消列表
        if (it.second->repeat() &&
            cancelingTimers_.find(timer) == cancelingTimers_.end())
        {
            it.second->restart(now);
            // 重新插入队列
            insert(it.second);
        }
        else 
        {
            // 一次性定时器或者已被取消，删除
            delete it.second;
        }
    }

    if (!timers_.empty())
    {
        // 获取最早到期的定时器超时时间
        nextExpried = timers_.begin()->second->expriation();
    }

    if (nextExpried.valid())
    {
        // 重置定时器的超时时刻
        resetTimerfd(timerfd_, nextExpried);
    }
}

bool TimerQueue::insert(Timer *timer)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    
    // 最早到期时间是否改变
    bool earliestChanged = false;

    Timestamp when = timer->expriation();
    TimerList::iterator it = timers_.begin();
    // 如果timers_为空，或者when小于timers_中的时间
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }

    // 插入到timers_中
    std::pair<TimerList::iterator, bool> result1
        = timers_.insert(Entry(when, timer));
    assert(result1.second); (void)result1;

    // 插入到activeTimers_中
    std::pair<ActiveTimerSet::iterator, bool> result2
        = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
    assert(result2.second); (void)result2;

    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}




 