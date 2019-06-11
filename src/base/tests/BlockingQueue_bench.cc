/*
 * @Author: py.wang 
 * @Date: 2019-05-09 08:19:17 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-09 08:43:46
 */

#include "src/base/BlockingQueue.h"
#include "src/base/CountDownLatch.h"
#include "src/base/Thread.h"
#include "src/base/Timestamp.h"

#include <map>
#include <string>
#include <vector>

#include <stdio.h>
#include <unistd.h>

class Bench
{
public:
    Bench(int numThreads)
        :latch_(numThreads)
    {
        threads_.reserve(numThreads);
        for (int i = 0; i < numThreads; ++i)
        {
            char name[32];
            snprintf(name, sizeof name, "work thread %d", i);
            threads_.emplace_back(new slack::Thread(std::bind(&Bench::threadFunc, this), slack::string(name)));
        }
        for (auto &thr : threads_)
        {
            thr->start();
        }
    }

    void run(int times)
    {
        printf("waiting for count down latch\n");
        latch_.wait();
        printf("all threads started\n");
        for (int i = 0; i < times; ++i)
        {
            slack::Timestamp now(slack::Timestamp::now());
            queue_.put(now);
            usleep(1000);
        }
    }

    void joinAll()
    {
        for (size_t i = 0; i < threads_.size(); ++i)
        {
            queue_.put(slack::Timestamp::invalid());
        }

        for (auto &thr : threads_)
        {
            thr->join();
        }
    }
private:
    void threadFunc()
    {
        printf("tid=%d, %s started\n",
                slack::CurrentThread::tid(),
                slack::CurrentThread::name());
        std::map<int, int> delays;
        latch_.countDown();
        bool running = true;
        while (running)
        {
            slack::Timestamp t(queue_.take());
            slack::Timestamp now(slack::Timestamp::now());
            if (t.valid())
            {
                int delay = static_cast<int>(slack::timeDifference(now, t) * slack::Timestamp::kMicroSecondsPerSecond);
                ++delays[delay];
            }
            running = t.valid();
        }
        printf("tid=%d, %s stopped\n",
                slack::CurrentThread::tid(),
                slack::CurrentThread::name());
        for (const auto &delay : delays)
        {
            printf("tid=%d, delay=%d, count=%d\n",
                    slack::CurrentThread::tid(),
                    delay.first, delay.second);
        }
        
    }

    slack::BlockingQueue<slack::Timestamp> queue_;
    slack::CountDownLatch latch_;
    std::vector<std::unique_ptr<slack::Thread>> threads_;
};

int main(int argc, char *argv[])
{
    int threads = argc > 1 ? atoi(argv[1]) : 1;

    Bench t(threads);
    t.run(1000);
    t.joinAll();
}