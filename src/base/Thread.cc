/*
 * @Author: py.wang 
 * @Date: 2019-05-05 09:28:23 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-09 16:58:02
 */

#include "src/base/Thread.h"
#include "src/base/Exception.h"
#include "src/base/CurrentThread.h"
#include "src/base/Timestamp.h"
//#include "src/log/Logging.h"

#include <type_traits>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace slack
{

namespace detail
{
    pid_t gettid()
    {
        // 获取全局唯一PID标识，glibc没提供,借助Linux系统调用
        return static_cast<pid_t>(::syscall(SYS_gettid));
    }

    void afterFork()
    {
        slack::CurrentThread::t_cachedTid = 0;
        slack::CurrentThread::t_threadName = "main";
        // cacheTid
        slack::CurrentThread::tid();
    }

    class ThreadNameInitializer
    {
    public:
        ThreadNameInitializer()
        {
            slack::CurrentThread::t_threadName = "main";
            slack::CurrentThread::tid();
            // FIXME
            // fork()的子进程只会继承main thread，同时继承mutex，如果不是main thread锁住，那么就不会被解锁 
            // 注册fork回调，万一fork就清空缓存，避免使用相同的缓存tid
            pthread_atfork(nullptr, nullptr, &afterFork);
        }
    };

    // 注册回调
    ThreadNameInitializer init;

    // prepare data
    struct ThreadData
    {
        using ThreadFunc = slack::Thread::ThreadFunc;
        ThreadFunc func_;
        string name_;
        pid_t *tid_;
        CountDownLatch *latch_;

        ThreadData(ThreadFunc func,
                    const string &name,
                    pid_t *tid,
                    CountDownLatch *latch)
            : func_(std::move(func)),
            name_(name),
            tid_(tid),
            latch_(latch)
        {
        }

        void runInThread()
        {
            // cache
            *tid_ = slack::CurrentThread::tid();
            tid_ = nullptr;
            latch_->countDown();
            latch_ = nullptr;

            slack::CurrentThread::t_threadName = name_.empty() ? "slackThread" : name_.c_str();
            // 设置调用线程的名字
            ::prctl(PR_SET_NAME, slack::CurrentThread::t_threadName);
            try
            {
                func_();
                slack::CurrentThread::t_threadName = "finished";   
            }
            catch (const Exception &ex)
            {
                slack::CurrentThread::t_threadName = "crashed";
                fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                fprintf(stderr, "reason: %s\n", ex.what());
                fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
                abort();
            }
            catch (const std::exception &ex)
            {
                slack::CurrentThread::t_threadName = "crashed";
                fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                fprintf(stderr, "reason: %s\n", ex.what());
                abort();
            }
            catch (...)
            {
                slack::CurrentThread::t_threadName = "crashed";
                fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
                throw;  // rethrow
            }
        }
    };

    // 线程函数
    void *startThread(void *obj)
    {
        ThreadData *data = static_cast<ThreadData *>(obj);
        data->runInThread();
        delete data;
        return nullptr;
    }

}   // namespace detail

/* CurrentThread function */
void CurrentThread::cachedTid()
{
    // lazy cache
    if (t_cachedTid == 0)
    {
        t_cachedTid = detail::gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
        assert(t_tidStringLength == 6);
    }
}

bool CurrentThread::isMainThread()
{
    // 线程名字与进程名字相同则为主线程
    return tid() == ::getpid();
}

void CurrentThread::sleepUsec(int64_t usec)
{
    struct timespec ts = {0, 0};
    ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond);
    ::nanosleep(&ts, nullptr);
}

/* Thread function */
AtomicInt32 Thread::numCreated_;    // 初始化为0

Thread::Thread(ThreadFunc func, const string &name)
    : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(0),
    func_(std::move(func)),
    name_(name),
    latch_(1)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        // no joined and detach
        pthread_detach(pthreadId_);
    }
}

void Thread::setDefaultName()
{
    int num = numCreated_.incrementAndGet();
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}

void Thread::start()
{
    assert(!started_);
    started_ = true;
    // FIXME: move func
    // prepare data
    detail::ThreadData *data = new detail::ThreadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthreadId_, nullptr, &detail::startThread, data) != 0)
    {
        // error
        started_ = false;
        delete data;
        // LOG_SYSFATAL << "Failed in pthread_create";
        abort();
    }
    else 
    {
        // 等待同步
        latch_.wait();
        assert(tid_ > 0);
    }
}

int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, nullptr);
}

}   // namespace slack


