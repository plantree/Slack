/*
 * @Author: py.wang 
 * @Date: 2019-05-24 09:31:43 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-25 08:10:05
 */
#ifndef NET_EVENTLOOPTHREAD_H
#define NET_EVENTLOOPTHREAD_H

#include "src/base/Condition.h"
#include "src/base/Mutex.h"
#include "src/base/Thread.h"

namespace slack
{

namespace net
{

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    typedef std::function<void(EventLoop *)> ThreadInitCallback;
    
    // 空函数调用
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const string &name = string());   
    ~EventLoopThread();

    EventLoop *startLoop();     // 启动线程，成为IO线程

private:
    void threadFunc();  // 线程函数

    EventLoop *loop_;   // 指向一个EventLoop对象
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    ThreadInitCallback callback_;   // 回调函数在loop之前被调用
};

}   // namespace net

}   // namespace slack



#endif  // NET_EVENTLOOPTHREAD_H