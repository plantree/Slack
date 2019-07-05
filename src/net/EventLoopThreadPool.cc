/*
 * @Author: py.wang 
 * @Date: 2019-05-30 08:30:53 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-30 09:08:41
 */
#include "src/net/EventLoopThreadPool.h"
#include "src/net/EventLoop.h"
#include "src/net/EventLoopThread.h"

#include <functional>

using namespace slack;
using namespace slack::net;

// ctor
EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const slack::string &nameArg)
    : baseLoop_(baseLoop),
    name_(nameArg),
    started_(false),
    numThreads_(0),
    next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // don't delete loop, it's stack variable
    // EventLoop指向栈对象，会自动析构
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    assert(!started_);
    baseLoop_->assertInLoopThread();

    started_ = true;

    // 建立EventLoop线程池
    for (int i = 0; i < numThreads_; ++i)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());   // 启动EventLoopThread线程，进入事件循环前会调用cb
    }

    if (numThreads_ == 0 && cb)
    {
        // 只有一个EventLoop，进入事件循环前调用cb
        cb(baseLoop_);
    }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    assert(started_);
    EventLoop *loop = baseLoop_;

    // 如果loops_为空，loop指向baseLoop
    // 否则按照round-robin(轮询)
    if (!loops_.empty())
    {
        // round-robin
        loop = loops_[next_++];
        if (implicit_cast<size_t>(next_) >= loops_.size())
        {
            next_ = 0;
        }
    }
    return loop;
}

EventLoop *EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
    baseLoop_->assertInLoopThread();
    EventLoop *loop = baseLoop_;

    if (!loops_.empty())
    {
        loop = loops_[hashCode % loops_.size()];
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    baseLoop_->assertInLoopThread();
    assert(started_);
    if (loops_.empty())
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else 
    {
        return loops_;
    }
}

