/*
 * @Author: py.wang 
 * @Date: 2019-05-02 08:42:52 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-05 16:35:23
 */

#ifndef BASE_TIMESTAMP_H
#define BASE_TIMESTAMP_H

#include "src/base/copyable.h"
#include "src/base/Types.h"

#include <algorithm>

namespace slack 
{

// UTC 时间戳，微秒精度
// 这个类不可变，推荐传值而非引用
class Timestamp : public slack::copyable
{
public:
    // 默认无效时间戳
    Timestamp() : microSecondsSinceEpoch_(0)
    {
    }

    // 给定时间时间戳
    // 避免隐式转换
    explicit Timestamp(int64_t microSecondsSinceEpoch)
        : microSecondsSinceEpoch_(microSecondsSinceEpoch)
    {
    }

    void swap(Timestamp &that) 
    {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }

    // 默认的拷贝初始化、赋值以及析构

    string toString() const;
    string toFormattedString(bool showMicroseconds = true) const;

    bool valid() const 
    {
        return microSecondsSinceEpoch_ > 0;
    }

    // 内部使用
    int64_t microSecondsSinceEpoch() const 
    {
        return microSecondsSinceEpoch_;
    }
    // 微秒转为秒
    time_t secondsSinceEpoch() const 
    {
        return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    }

    // get time of now
    static Timestamp now();
    // 返回一个无效时间戳
    static Timestamp invalid()
    {
        return Timestamp();
    }
    static Timestamp fromUnixTime(time_t t)
    {
        return fromUnixTime(t, 0);
    }
    static Timestamp fromUnixTime(time_t t, int microSeconds)
    {
        return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microSeconds);
    }
    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) 
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

// 获取时间差，单位(秒)
inline double timeDifference(Timestamp high, Timestamp low)
{
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

// 增加时间
inline Timestamp addTime(Timestamp timestamp, double seconds) 
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

} // namespace slack

#endif  // BASE_TIMESTAMP_H