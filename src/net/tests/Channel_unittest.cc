/*
 * @Author: py.wang 
 * @Date: 2019-05-23 09:58:44 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-24 08:01:42
 */

#include "src/log/Logging.h"
#include "src/net/Channel.h"
#include "src/net/EventLoop.h"

#include <functional>
#include <map>

#include <stdio.h>
#include <unistd.h>
#include <sys/timerfd.h>

using namespace slack;
using namespace slack::net;

void print(const char *msg)
{
    static std::map<const char *, Timestamp> lasts;
    Timestamp &last = lasts[msg];
    Timestamp now = Timestamp::now();
    printf("%s tid %d %s delay %f\n", now.toString().c_str(), CurrentThread::tid(),
            msg, timeDifference(now, last));
    last = now;
}

namespace slack
{

namespace net
{

namespace detail
{
int createTimerfd();
void readTimerfd(int timerfd, Timestamp now);

}   // namespace detail

}   // namespace net

}   // namespace slack

// use relative time, immunized to wall clock changes
class PeriodicTimer
{
public:
    PeriodicTimer(EventLoop *loop, double interval, const TimerCallback &cb)
        : loop_(loop),
        timerfd_(slack::net::detail::createTimerfd()),
        timerfdChannel_(loop, timerfd_),
        interval_(interval),
        cb_(cb)
    {
        timerfdChannel_.setReadCallback(std::bind(&PeriodicTimer::handleRead, this));
        timerfdChannel_.enableReading();
    }

    void start()
    {
        struct itimerspec spec;
        memZero(&spec, sizeof spec);
        spec.it_interval = toTimeSpec(interval_);
        spec.it_value = spec.it_interval;
        // 设置定时器
        int ret = ::timerfd_settime(timerfd_, 0/* relative timer*/, &spec, nullptr);
        if (ret)
        {
            LOG_SYSERR << "timerfd_settime()";
        }
    }

    ~PeriodicTimer()
    {
        timerfdChannel_.disableAll();
        timerfdChannel_.remove();
        ::close(timerfd_);
    }
private:
    void handleRead()
    {
        loop_->assertInLoopThread();
        slack::net::detail::readTimerfd(timerfd_, Timestamp::now());
        if (cb_)
        {
            cb_();
        }
    }

    static struct timespec toTimeSpec(double seconds)
    {
        struct timespec ts;
        memZero(&ts, sizeof ts);
        const int64_t kNanoSecondsPerSecond = 1000000000;
        const int kMinInterval = 100000;
        int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);
        if (nanoseconds < kMinInterval)
        {
            nanoseconds = kMinInterval;
        }
        ts.tv_sec = static_cast<time_t>(nanoseconds / kNanoSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(nanoseconds % kNanoSecondsPerSecond);
        return ts;
    }

    EventLoop *loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    const double interval_; // in seconds
    TimerCallback cb_;
};

int main()
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid()
        << " Try adjusting the wall clock, see what happeds.";
    EventLoop loop;
    PeriodicTimer timer(&loop, 1, std::bind(print, "PeriodicTimer"));
    // set
    timer.start();
    loop.runEvery(1, std::bind(print, "EventLoop::runEvery"));
    
    loop.loop();
}