/*
 * @Author: py.wang 
 * @Date: 2019-05-09 08:47:58 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-09 09:02:29
 */

#ifndef BASE_BOUNDEDBLOCKINGQUEUE_H
#define BASE_BOUNDEDBLOCKINGQUEUE_H

#include "src/base/Condition.h"
#include "src/base/Mutex.h"

#include <deque>

#include <assert.h>

namespace slack
{

template <typename T>
class BoundedBlockingQueue : noncopyable
{
public:
    explicit BoundedBlockingQueue(int maxSize)
        : mutex_(),
        notEmpty_(mutex_),
        notFull_(mutex_),
        capacity_(maxSize)
    {
    }

    void put(const T &x)
    {
        MutexLockGuard lock(mutex_);
        while (queue_.size() == capacity_)
        {
            notFull_.wait();
        }
        assert(queue_.size() < capacity_);
        queue_.push_back(x);
        notEmpty_.notify();     // TODO: move outsize of lock
    }
    
    void put(T &&x)
    {
        MutexLockGuard lock(mutex_);
        while (queue_.size() == capacity_)
        {
            notFull_.wait();
        }
        assert(queue_.size() < capacity_);
        queue_.push_back(std::move(x));
        notEmpty_.notify();    
    }

    T take()
    {
        MutexLockGuard lock(mutex_);
        while (queue_.empty())
        {
            notEmpty_.wait();   
        }
        assert(!queue_.empty());
        T front(std::move(queue_.front()));
        queue_.pop_front();
        notFull_.notify();
        return front;
    }

    bool empty() const
    {
        MutexLockGuard lock(mutex_);
        return queue_.empty();
    }

    bool full() const
    {
        MutexLockGuard lock(mutex_);
        return queue_.size() == capacity_;
    }

    size_t size() const
    {
        MutexLockGuard lock(mutex_);
        return queue_.size();
    }

    size_t capacity() const
    {
        MutexLockGuard lock(mutex_);
        return capacity_;
    }
private:
    mutable MutexLock mutex_;
    Condition notEmpty_;
    Condition notFull_;
    size_t capacity_;
    std::deque<T> queue_;
};

} // namespace slack

#endif  // BASE_BOUNDEDBLOCKINGQUEUE_H
