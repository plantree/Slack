/*
 * @Author: py.wang 
 * @Date: 2019-05-07 09:22:53 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-07 09:26:25
 */

#include "src/base/CountDownLatch.h"

using namespace slack;

// initialize
CountDownLatch::CountDownLatch(int count)
    : mutex_(),
    condition_(mutex_),
    count_(count)
{
}

// 条件变量的正确使用
void CountDownLatch::wait()
{
    MutexLockGuard lock(mutex_);
    // 避免假唤醒，用while
    while (count_ > 0)
    {
        condition_.wait();
    }
}

void CountDownLatch::countDown()
{
    MutexLockGuard lock(mutex_);
    --count_;
    // 通知
    if (count_ == 0)
    {
        condition_.notifyAll();
    }
}

int CountDownLatch::getCount() const
{
    MutexLockGuard lock(mutex_);
    return count_;
}
