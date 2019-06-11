/*
 * @Author: py.wang 
 * @Date: 2019-05-09 07:46:39 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-09 08:14:13
 */

#ifndef BASE_BLOCKINGQUEUE_H
#define BASE_BLOCKINGQUEUE_H

#include "src/base/Condition.h"
#include "src/base/Mutex.h"

#include <deque>

#include <assert.h>

/*****************************
 * 无界队列
 *****************************/
namespace slack
{

template <typename T>
class BlockingQueue : noncopyable
{
public:
    BlockingQueue()
        : mutex_(),
        notEmpty_(mutex_),
        queue_()
    {
    }

    // 生产者
    void put(const T &x)
    {
        MutexLockGuard lock(mutex_);
        queue_.push_back(x);
        notEmpty_.notify();     // TODO: move outside of lock
    }

    void put(T &&x)
    {
        MutexLockGuard lock(mutex_);
        queue_.push_back(std::move(x));
        notEmpty_.notify();
    }

    // 消费者
    T take()
    {
        MutexLockGuard lock(mutex_);
        // always use a while loop, due to spurious wakeup
        while (queue_.empty())
        {
            notEmpty_.wait();
        }
        assert(!queue_.empty());
        T front(std::move(queue_.front()));
        queue_.pop_front();
        return front;
    }

    size_t size() const
    {
        MutexLockGuard lock(mutex_);
        return queue_.size();
    }
private:
    mutable MutexLock   mutex_;
    Condition           notEmpty_;
    std::deque<T>       queue_;
};

}   // namespace slack

#endif  // BASE_BLOCKING_QUEUE_H