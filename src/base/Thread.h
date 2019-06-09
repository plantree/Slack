/*
 * @Author: py.wang 
 * @Date: 2019-05-05 09:21:27 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-08 21:04:59
 */

#ifndef BASE_THREAD_H
#define BASE_THREAD_H

#include "src/base/Atomic.h"
#include "src/base/Types.h"
#include "src/base/CountDownLatch.h"

#include <functional>
#include <memory>

#include <pthread.h>

namespace slack
{

class Thread : noncopyable
{
public:
    // 回调函数
    typedef std::function<void ()> ThreadFunc;
    // move threadFunc
    // 回调函数和线程名字
    explicit Thread(ThreadFunc, const string &name = string());
    ~Thread();

    void start();
    int join();     // return pthread_join

    bool started() const { return started_; }
    pthread_t pthreadId() const { return pthreadId_; }
    pid_t tid() const { return tid_; }
    const string &name() const { return name_; }

    static int numCreated() { return numCreated_.get(); }

private:
    void setDefaultName();

    bool started_;
    bool joined_;
    pthread_t pthreadId_;   // 线程库id
    pid_t tid_;             // 内核线程id，其实也是进程id
    ThreadFunc func_;       // 回调函数
    string name_;           // 线程名字
    CountDownLatch latch_;  // 同步工具
    
    // 线程计数，类共享
    static AtomicInt32 numCreated_;
};

}   // namespace slack

#endif  // BASE_THREAD_H