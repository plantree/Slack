/*
 * @Author: py.wang 
 * @Date: 2019-05-25 07:55:34 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-25 08:22:33
 */

#include "src/net/EventLoopThread.h"
#include "src/net/EventLoop.h"

#include <functional>

using namespace slack;
using namespace slack::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                const string &name)
    : loop_(nullptr),
    exiting_(false),
    thread_(std::bind(&EventLoopThread::threadFunc, this), name),
    mutex_(),
    cond_(mutex_),
    callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_) // not 100% race-free, eg. threadFunc could be running callback_
    {
        loop_->quit();  // 退出IO线程
        thread_.join(); // 等待销毁
    }
}

// 开启事件循环
EventLoop *EventLoopThread::startLoop()
{
    assert(!thread_.started());

    thread_.start();    // 新启线程，执行初始化回调

    EventLoop *loop = nullptr;
    {
        MutexLockGuard lock(mutex_);
        // 等待线程函数设置EventLoop
        // 保证在threadFunc回调之后才会返回
        while (!loop_)
        {
            cond_.wait();
        }
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;

    // 先执行回调，然后启动后loop
    if (callback_)
    {
        callback_(&loop);
    }

    {
        MutexLockGuard lock(mutex_);
        // loop_指针指向一个栈上对象，threadFunc退出后指针就失效
        // threadFunc退出，意味着线程退出，EventLoopThread就没有存在价值
        loop_ = &loop;
        cond_.notify();
    }

    loop.loop();
    MutexLockGuard lock(mutex_);
    loop_ = nullptr;
}