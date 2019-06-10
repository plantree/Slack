/*
 * @Author: py.wang 
 * @Date: 2019-05-09 09:29:42 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-09 19:52:54
 */

#include "src/base/ThreadPool.h"
#include "src/base/Exception.h"

#include <functional>

#include <assert.h>
#include <stdio.h>

using namespace slack;

ThreadPool::ThreadPool(const string &name)
    : mutex_(),
    notEmpty_(mutex_),
    notFull_(mutex_),
    name_(name),
    maxQueueSize_(0),
    running_(false)
{
}

ThreadPool::~ThreadPool()
{
    if (running_)
    {
        stop();
    }
}

void ThreadPool::start(int numThreads)
{
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(numThreads);
    // create threads
    for (int i = 0; i < numThreads; ++i)
    {
        char id[32];
        snprintf(id, sizeof id, "%d", i+1);
        threads_.emplace_back(new slack::Thread(
            std::bind(&ThreadPool::runInThread, this), name_+id));
        threads_[i]->start();
    }
    // 没有线程
    if (numThreads == 0 && threadInitCallback_)
    {
        threadInitCallback_();
    }
}


void ThreadPool::stop()
{
    {
        MutexLockGuard lock(mutex_);
        running_ = false;
        // 通知所有线程赶紧消费
        notEmpty_.notifyAll();
    }
    for (auto &thr : threads_)
    {
        // 等待其他线程结束
        thr->join();
    }
}

size_t ThreadPool::queueSize() const 
{
    MutexLockGuard lock(mutex_);
    return queue_.size();
}

void ThreadPool::run(Task task)
{
    // 线程池为空则直接运行
    if (threads_.empty())
    {
        task();
    }
    else
    {
        //printf("test2 lock: tid=%d, holder=%d\n", CurrentThread::tid(), mutex_.getHolder());
        // 放入任务队列
        MutexLockGuard lock(mutex_);
    
        while (isFull())
        {
            // 任务队列满，等待消费
            notFull_.wait();
        }
        assert(!isFull());

        queue_.push_back(std::move(task));
        // 只通知一个线程消费
        notEmpty_.notify();
    }
}

ThreadPool::Task ThreadPool::take()
{
    MutexLockGuard lock(mutex_);

    // always use a while loop, due to spurious wakeup
    while (queue_.empty() && running_)  // 队列不为空或者不运行都会终止等待
    {
        notEmpty_.wait();
    }
    Task task;
    if (!queue_.empty())
    {
        task = queue_.front();
        queue_.pop_front();
        if (maxQueueSize_ > 0)
        {
            // 通知生产者
            notFull_.notify();
        }
    }
    return task;
}

bool ThreadPool::isFull() const 
{
    mutex_.assertLocked();
    // 已经锁住，在互斥区
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
    try
    {
        if (threadInitCallback_)
        {
            threadInitCallback_();
        }
        // 消费者
        while (running_)
        {
            Task task(take());
            if (task)
            {
                task();
            }
        }
    }
    catch(const Exception &ex)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n" ,ex.stackTrace());
        abort();
    }
    catch (const std::exception &ex)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch (...)
    {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw;  // rethrow
    }
    
}

