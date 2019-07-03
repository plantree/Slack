/*
 * @Author: py.wang 
 * @Date: 2019-05-22 08:05:46 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-23 09:06:55
 */
#ifndef NET_TIMERQUEUE_H
#define NET_TIMERQUEUE_H

#include "src/base/Mutex.h"
#include "src/base/Timestamp.h"
#include "src/net/Callbacks.h"
#include "src/net/Channel.h"

#include <set>
#include <vector>

namespace slack
{
    
namespace net
{
// 前向声明
class EventLoop;
class Timer;
class TimerId;

// A best efforts timer queue.
// No guarantee that the callback will be on time
class TimerQueue : noncopyable
{
public:
    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();

    // schedules the callback to be run at given time,
    // repeats if interval_ > 0.0
    //
    // Must be thread safe. Usually be called from other threads
    // 对于封装了Timer的TimerId操作
    TimerId addTimer(TimerCallback cb,
                    Timestamp when,
                    double interval);
    void cancel(TimerId timerId);

private:
    // (Timestamp, *Timer)
    typedef std::pair<Timestamp, Timer *> Entry;
    // 按时间戳先后顺序排序（红黑树）
    typedef std::set<Entry> TimerList;
    // (Timer *, Sequence)，有效定时器
    typedef std::pair<Timer *, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    // 该函数只能在所属IO线程中(EventLoop)调用，因此不必加锁，避免了锁竞争
    void addTimerInLoop(Timer *timer);
    void cancelInLoop(TimerId timerId);

    // called when timerfd alarms
    void handleRead();
    // move out all expired timers
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry> &expired, Timestamp now);

    // 辅助插入定时器函数
    bool insert(Timer *timer);

    // members
    EventLoop *loop_;
    const int timerfd_;
    Channel timerfdChannel_;    // 绑定到定时器描述符
    // Timer list sorted by expiration
    TimerList timers_;

    // for cancel()
    ActiveTimerSet activeTimers_;
    bool callingExpiredTimers_;     // atomic
    ActiveTimerSet cancelingTimers_;
};

}   // namespace net

}   // namespace slack


#endif // NET_TIMERQUEUE_H