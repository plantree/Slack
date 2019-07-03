/*
 * @Author: py.wang 
 * @Date: 2019-05-22 08:00:47 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-22 08:02:23
 */

#include "src/net/Timer.h"

using namespace slack;
using namespace slack::net;

// 初始为0
AtomicInt64 Timer::s_numCreated_;

void Timer::restart(Timestamp now)
{
    if (repeat_)
    {
        // 重新计算下一个超时时刻
        expriation_ = addTime(now, interval_);
    }
    else 
    {
        // 否则设置为无效
        expriation_ = Timestamp::invalid();
    }
}