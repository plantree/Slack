/*
 * @Author: py.wang 
 * @Date: 2019-05-29 09:18:50 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-30 08:34:07
 */
#ifndef NET_EVENTLOOPTHREADPOOL_H
#define NET_EVENTLOOPTHREADPOOL_H

#include "src/base/Condition.h"
#include "src/base/Mutex.h"

#include <vector>
#include <functional>
#include <memory>

namespace slack
{

namespace net
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    typedef std::function<void(EventLoop *)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop *baseLoop, const string &nameArg);
    ~EventLoopThreadPool();
    
    void setThreadNum(int numThreads) 
    {
        numThreads_ = numThreads;
    }
    void start(const ThreadInitCallback &cb = ThreadInitCallback());
   
    // valid after calling start()
    // round-robin
    EventLoop *getNextLoop();

    // with the same hash code, it will always return the same EventLoop
    EventLoop * getLoopForHash(size_t hashCode);

    std::vector<EventLoop *> getAllLoops();

    bool started() const 
    {
        return started_;
    }

    slack::string name() const 
    {
        return name_;
    }

private:
    // 与Acceptor中的EventLoop一致
    EventLoop *baseLoop_;
    slack::string name_;
    bool started_;
    int numThreads_;
    int next_;  // 新连接到来，选择的EventLoop对象下标

    std::vector<std::unique_ptr<EventLoopThread>> threads_; // IO线程池
    std::vector<EventLoop *> loops_;
};

}   // namespace net

}   // namespace slack


#endif  // NET_EVENTLOOPTHREADPO