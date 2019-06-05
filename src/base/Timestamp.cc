/*
 * @Author: py.wang 
 * @Date: 2019-05-02 09:13:23 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-05 16:35:13
 */

#include "src/base/Timestamp.h"

#include <sys/time.h>
#include <stdio.h>

// 为了使用 inttypes.h，跨平台
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

using namespace slack;

// 编译期断言
static_assert(sizeof(Timestamp) == sizeof(int64_t), 
              "Timestamp is same size as int64_t");


string Timestamp::toString() const
{
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microSeconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds, microSeconds);
    return buf;
}

string Timestamp::toFormattedString(bool showMicroseconds) const 
{
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    // 分解时间
    struct tm tm_time;
    // 可重入,线程安全,将时间戳转换为当地日期
    localtime_r(&seconds, &tm_time);

    if (showMicroseconds) 
    {
        int microSeconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microSeconds);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
        tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
        tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
}

Timestamp Timestamp::now()
{
    struct timeval tv;
    // 获取微秒级精度
    gettimeofday(&tv, nullptr);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}
