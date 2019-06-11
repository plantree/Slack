/*
 * @Author: py.wang 
 * @Date: 2019-05-09 07:54:38 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-09 08:17:58
 */

#include "src/base/BlockingQueue.h"
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
        :latch_(numThreads)
    {
        // create threads
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
        // 等待工作线程开启
        latch_.wait();
        printf("all threads started\n");
        for (int i = 0; i < times; ++i)
        {
            char buf[32];
            snprintf(buf, sizeof buf, "hello %d", i);
            // 生产者生产
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
        // 停止标识
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

    slack::BlockingQueue<std::string> queue_;
    slack::CountDownLatch latch_;
    std::vector<std::unique_ptr<slack::Thread>> threads_;
};

void testMove()
{
    slack::BlockingQueue<std::unique_ptr<int>> queue;
    queue.put(std::unique_ptr<int>(new int (42)));
    std::unique_ptr<int> x = queue.take();
    printf("took %d\n", *x);
    *x = 123;
    queue.put(std::move(x));
    std::unique_ptr<int> y = queue.take();
    printf("took %d\n", *y);
}

int main()
{
    printf("pid=%d, tid=%d\n", ::getpid(), slack::CurrentThread::tid());
    Test t(5);
    t.run(100);
    t.joinAll();

    testMove();

    printf("number of created threads %d\n", slack::Thread::numCreated());
}