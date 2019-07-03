/*
 * @Author: py.wang 
 * @Date: 2019-05-21 09:42:49 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-22 09:09:20
 */

#ifndef NET_TIMERID_H
#define NET_TIMERID_H

#include "src/base/copyable.h"

namespace slack
{

namespace net
{

// 前置声明
class Timer;

// An opaque identifier, for canceling Timer
class TimerId : public copyable
{
    friend class TimerQueue;
public:
    TimerId()
        : timer_(nullptr),
        sequence_(0)
    {
    }

    TimerId(Timer *timer, int64_t seq)
        : timer_(timer),
        sequence_(seq)
    {
    }

    // default copy-ctor, dtor and assignment are okay
    // FIXME
    // 底层共享一份资源，内存泄露???
private:
    Timer *timer_;      // 定时器
    int64_t sequence_;  // 序列
};

}   // namespace net

}   // namespace slack

#endif  // NET_TIMERID_H