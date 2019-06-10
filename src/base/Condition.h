/*
 * @Author: py.wang 
 * @Date: 2019-05-07 09:11:25 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-16 09:37:15
 */

#ifndef BASE_CONDITION_H
#define BASE_CONDITION_H

#include "src/base/Mutex.h"

#include <pthread.h>

namespace slack
{
    
class Condition : noncopyable
{
public:
    // mutex_引用传递
    explicit Condition(MutexLock &mutex) : mutex_(mutex)
    {
        int ret = pthread_cond_init(&pcond_, nullptr);
        assert(ret == 0); (void)ret;
    }

    ~Condition()
    {
        int ret = pthread_cond_destroy(&pcond_);
        assert(ret == 0); (void) ret;
    }

    // 等待条件变量
    void wait()
    {
        MutexLock::UnassignGuard ug(mutex_);
        pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
    }

    // returns true if time out, false otherwisw
    bool waitForSeconds(double seconds);

    // 通知
    void notify()
    {
        pthread_cond_signal(&pcond_);
    }

    void notifyAll()
    {
        pthread_cond_broadcast(&pcond_);
    }
private:
    MutexLock &mutex_;
    pthread_cond_t pcond_;
};

} // namespace slack


#endif  // BASE_CONDITION_H