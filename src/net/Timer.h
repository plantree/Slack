/*
 * @Author: py.wang 
 * @Date: 2019-05-22 07:50:45 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-22 08:06:42
 */

#ifndef NET_TIMER_H
#define NET_TIMER_H

#include "src/base/noncopyable.h"
#include "src/base/Atomic.h"
#include "src/net/Callbacks.h"

namespace slack
{

namespace net
{

// Internal class for timer event
class Timer : noncopyable
{
public:
    Timer(TimerCallback cb, Timestamp when, double interval)
        : callback_(std::move(cb)),
        expriation_(when),  
        interval_(interval),
        repeat_(interval_ > 0.0),
        sequence_(s_numCreated_.incrementAndGet())
    {
    }

    void run() const 
    {
        callback_();
    }

    // 失效时间
    Timestamp expriation() const 
    {
        return expriation_;
    }

    bool repeat() const 
    {
        return repeat_;
    }

    int64_t sequence() const 
    {
        return sequence_;
    }

    // 重启
    void restart(Timestamp now);

    static int64_t numCreated()
    {
        return s_numCreated_.get();
    }
    
private:
    const TimerCallback callback_;  // 定时器回调函数
    Timestamp expriation_;          // 下一次的超时时刻
    const double interval_;         // 超时时间间隔，一次性的话为0
    const bool repeat_;             // 是否重复
    const int64_t sequence_;        // 定时器序号
    
    // 全局
    static AtomicInt64 s_numCreated_;   // 定时器计数
};

}   // net

}   // slack

#endif  // NET_TIMER_H