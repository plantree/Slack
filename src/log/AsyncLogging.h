/*
 * @Author: py.wang 
 * @Date: 2019-05-16 08:56:19 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-16 09:37:00
 */

#ifndef LOG_ASYNCLOGGING_H
#define LOG_ASYNCLOGGING_H

#include "src/base/BlockingQueue.h"
#include "src/base/BoundedBlockingQueue.h"
#include "src/base/CountDownLatch.h"
#include "src/base/Mutex.h"
#include "src/base/Thread.h"
#include "src/base/Condition.h"
#include "src/log/LogStream.h"

#include <atomic>
#include <vector>

namespace slack
{

class AsyncLogging : noncopyable
{
public:
    AsyncLogging(const string &basename,
                off_t rollSize,
                int flushInterval = 3);
    ~AsyncLogging()
    {
        if (running_)
        {
            stop();
        }
    }

    void append(const char *logline, int len);

    void start()
    {
        running_ = true;
        thread_.start();
        // 在线程运行之后返回
        latch_.wait();
    }

    void stop()
    {
        running_ = false;
        // 通知后端写
        cond_.notify();
        thread_.join();
    }

private:
    void threadFunc();
    // 缓冲区
    using Buffer = slack::detail::FixedBuffer<slack::detail::kLargeBuffer>;
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;

    const int flushInterval_;
    std::atomic<bool> running_;
    const string basename_;
    const off_t rollSize_;

    slack::Thread thread_;      // 日志线程
    slack::CountDownLatch latch_;
    slack::MutexLock mutex_;
    slack::Condition cond_;
    BufferPtr currentBuffer_;   // 当前缓冲区
    BufferPtr nextBuffer_;      // 后备缓冲区
    BufferVector buffers_;
};

}   // namespace slack

#endif  // LOG_ASYNCLOGGING_H
