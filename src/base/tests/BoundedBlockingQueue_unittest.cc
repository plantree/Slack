/*
 * @Author: py.wang 
 * @Date: 2019-05-09 07:54:38 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-09 09:01:35
 */

#include "src/base/BoundedBlockingQueue.h"
#include "src/base/Thread.h"
#include "src/base/CountDownLatch.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <stdio.h>
#include <unistd.h>

class Test
{
public:
    Test(int numThreads)
        : queue_(10),
        latch_(numThreads)
    {
        threads_.reserve(numThreads);
        for (int i = 0; i < numThreads; ++i)
        {
            char name[32];
            snprintf(name, sizeof name, "work thread %d", i);
            threads_.emplace_back(new slack::Thread(std::bind(&Test::threadFunc, this), slack::string(name)));
        }
        // Thread不可拷贝
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
        // 生产者生产
        for (int i = 0; i < times; ++i)
        {
            char buf[32];
            snprintf(buf, sizeof buf, "hello %d", i);
            queue_.put(buf);
            printf("tid=%d, put data=%s, size=%zd\n", slack::CurrentThread::tid(), buf, queue_.size());
        }
    }

    void joinAll()
    {
        for (size_t i = 0; i < threads_.size(); ++i)
        {
            // 设置终点
            queue_.put("stop");
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
        
        latch_.countDown();
        bool running = true;
        // 消费
        while (running)
        {
            std::string d(queue_.take());
            printf("tid=%d, get data=%s, size=%zd\n", slack::CurrentThread::tid(), d.c_str(), queue_.size());
            running = (d != "stop");
        }

        printf("tid=%d, %s stopped\n",
                slack::CurrentThread::tid(),
                slack::CurrentThread::name());
    }

    slack::BoundedBlockingQueue<std::string> queue_;
    slack::CountDownLatch latch_;
    std::vector<std::unique_ptr<slack::Thread>> threads_;
};

int main()
{
    printf("pid=%d, tid=%d\n", ::getpid(), slack::CurrentThread::tid());
    Test t(1);
    t.run(100);
    t.joinAll();

    printf("number of created threads %d\n", slack::Thread::numCreated());
}