/*
 * @Author: py.wang 
 * @Date: 2019-05-07 08:30:29 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-08 20:27:45
 */

#ifndef BASE_MUTEX_H
#define BASE_MUTEX_H

#include "src/base/noncopyable.h"
#include "src/base/CurrentThread.h"

#include <assert.h>
#include <pthread.h>

namespace slack
{

// RAII
class MutexLock : noncopyable 
{
public:
    MutexLock() : holder_(0)
    {
        int ret = pthread_mutex_init(&mutex_, nullptr);
        assert(ret == 0); (void)ret;
    }

    // cannot copy-ctor and assignment

    ~MutexLock()
    {
        // 必须在没有线程持有锁的时候才能销毁
        assert(holder_ == 0);
        int ret = pthread_mutex_destroy(&mutex_);
        assert(ret == 0); (void)ret;
    }

    // 判断是否自己持有锁
    bool isLockedByThisThread()
    {
        return holder_ == CurrentThread::tid();
    }
    
    // 必须锁住
    void assertLocked()
    {
        assert(isLockedByThisThread());
    }

    // internal usage
    void lock()
    {
        pthread_mutex_lock(&mutex_);
        holder_ = CurrentThread::tid();
    }

    void unlock()
    {
        // 逆序
        holder_ = 0;
        pthread_mutex_unlock(&mutex_);
    }

    // 获取内部锁
    pthread_mutex_t *getPthreadMutex()
    {
        return &mutex_;
    }
private:
    pthread_mutex_t mutex_;     // 互斥锁
    pid_t holder_;              // 持有锁的线程ID

    void unassignHolder()
    {
        holder_ = 0;
    }
};

// 避免直接的对锁进行操作，lock/unlock必须对应，遇到异常可能出错
// 借助对象管理资源RAII
class MutexLockGuard : noncopyable
{
public:
    explicit MutexLockGuard(MutexLock &mutex) : mutex_(mutex)
    {
        mutex_.lock();
    }

    ~MutexLockGuard()
    {
        mutex_.unlock();
    }
private:
    MutexLock &mutex_;      // mutex_是不可拷贝的！！！不负责管理生存周期
};

// prevent misuse like
// MutexLockGurad(mutex_);
// A tempory object doesn't hold the lock for long
// 临时对象会报错
#define MutexLockGuard(x) error "Missing guard object name"

} // namespace slack


#endif  // BASE_MUTEX_H