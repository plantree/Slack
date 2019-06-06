/*
 * @Author: py.wang 
 * @Date: 2019-05-04 08:01:16 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-05 20:25:09
 */

// TODO: C++11 Atomic g++ atomic

#ifndef BASE_ATOMIC_H
#define BASE_ATOMIC_H

#include "src/base/noncopyable.h"

#include <atomic>

#include <stdint.h>

namespace slack
{

// 不暴露给外部，但又放在头文件里，供内部使用的函数或类所在的命名空间
namespace detail
{
// cannot be copied
template <typename T>
class AtomicIntegerT : noncopyable
{
public:
    AtomicIntegerT() : value_(0)
    {
    }

    T get()
    {
        //return __sync_val_compare_and_swap(&value_, 0, 0);
        return value_.load();

    }

    // 返回原值
    T getAndAdd(T x)
    {
        //return __sync_fetch_and_add(&value_, x);
        return value_.fetch_add(x);
    }

    T addAndGet(T x)
    {
        //return getAndAdd(x) + x;
        return value_ += x;
    }

    T incrementAndGet()
    {
        //return addAndGet(1);
        return ++value_;
    }

    T decrementAndGet()
    {
        //return addAndGet(-1);
        return --value_;
    }

    void add(T x)
    {
        //getAndAdd(x);
        return value_ += x;
    }

    void increment()
    {
        //incrementAndGet();
        ++value_;
    }

    void decrement()
    {
        //decrementAndGet();
        --value_;
    }

    // 返回原值
    T getAndSet(T newValue)
    {
        //return __sync_lock_test_and_set(&value_, newValue);
        return value_.exchange(newValue);
    }

private:
    // FIXME，这里的volatile并不是保证原子性
    //volatile T value_;
    volatile std::atomic<T> value_;

};

}   // namespace detail

using AtomicInt32 = detail::AtomicIntegerT<int32_t>;
using AtomicInt64 = detail::AtomicIntegerT<int64_t>;

}   // namespace slack

#endif  // BASE_ATOMIC_H