/*
 * @Author: py.wang 
 * @Date: 2019-05-07 09:20:14 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-08 09:11:42
 */

#ifndef BASE_COUNTDOWNLATCH_H
#define BASE_COUNTDOWNLATCH_H

#include "src/base/Condition.h"
#include "src/base/Mutex.h"

namespace slack
{

// 倒数门栓，允许一个线程或多个线程等待其他线程完成
class CountDownLatch : noncopyable
{
public:
    explicit CountDownLatch(int count);

    void wait();

    void countDown();

    int getCount() const;
    
private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
};

}   // namespace slack

#endif  // BASE_COUNTDOWNLATCH_H