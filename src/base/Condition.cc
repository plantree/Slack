/*
 * @Author: py.wang 
 * @Date: 2019-05-07 09:16:21 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-16 09:37:19
 */

#include "src/base/Condition.h"

#include <errno.h>

// return true if time out
bool slack::Condition::waitForSeconds(double seconds)
{
    struct timespec abstime;
    // 系统级真实时间
    // FIXME
    //clock_gettime(CLOCK_REALTIME, &abstime);
    clock_gettime(CLOCK_MONOTONIC, &abstime);
    
    const int64_t kNanoSecondsPerSecond = 1000000000;
    int64_t nanoSeconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);

    abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoSeconds) / kNanoSecondsPerSecond);
    abstime.tv_nsec += static_cast<long>((abstime.tv_nsec + nanoSeconds) % kNanoSecondsPerSecond);
    
    return pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime) == ETIMEDOUT;
}