/*
 * @Author: py.wang 
 * @Date: 2019-05-09 09:23:06 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-09 19:47:21
 */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "src/base/Condition.h"
#include "src/base/Mutex.h"
#include "src/base/Thread.h"
#include "src/base/Types.h"

#include <deque>
#include <vector>
#include <functional>

namespace slack
{

class ThreadPool : noncopyable
{
public:
    // 函数对象，线程任务
    typedef std::function<void()> Task;

    explicit ThreadPool(const string &nameArg = string("ThreadPool"));
    ~ThreadPool();

    // Must be called before start()
    void setMaxQueueSize(int maxSize)
    {
        maxQueueSize_ = maxSize;
    }
    void setThreadInitCallback(const Task &cb)
    {
        threadInitCallback_ = cb;
    }

    // 线程数量
    void start(int numThreads);
    void stop();

    const string &name() const 
    {
        return name_;
    }

    size_t queueSize() const;

    void run(Task f);

private:
    bool isFull() const;
    // 线程回调函数
    void runInThread();
    Task take();

    mutable MutexLock mutex_;
    // 生产者、消费者模型
    Condition notEmpty_;
    Condition notFull_;

    string name_;
    Task threadInitCallback_;
    // 线程池
    std::vector<std::unique_ptr<Thread>> threads_;
    // 任务队列
    std::deque<Task> queue_;
    size_t maxQueueSize_;
    bool running_;
};

}   // namespace slack

#endif  // THREADPOOL_H