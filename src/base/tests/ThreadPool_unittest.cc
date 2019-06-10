/*
 * @Author: py.wang 
 * @Date: 2019-05-10 08:06:54 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-10 08:16:22
 */

#include "src/base/ThreadPool.h"
#include "src/base/CountDownLatch.h"
#include "src/base/CurrentThread.h"

#include <functional>
#include <string>

#include <stdio.h>
#include <unistd.h> // usleep

void print()
{
    printf("tid=%d\n", slack::CurrentThread::tid());
}

void printString(const std::string &str)
{
    printf("tid=%d, str=%s\n", slack::CurrentThread::tid(), str.c_str());
    //usleep(1000 * 1000);
}

void test(int maxSize)
{
    printf("Test ThreadPool with max queue size = %d\n", maxSize);
    slack::ThreadPool pool("MainThreadPool");
    printf("Main thread: %d\n", slack::CurrentThread::tid());
    pool.setMaxQueueSize(maxSize);
    pool.start(5);

    printf("Adding\n");
    pool.run(print);
    pool.run(print);
    for (int i = 0;i < 100; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "task %d", i);
        pool.run(std::bind(printString, std::string(buf)));
    }
    printf("Done\n");

    slack::CountDownLatch latch(1);
    pool.run(std::bind(&slack::CountDownLatch::countDown, &latch));
    latch.wait();
    pool.stop();
    printf("created %d threads\n", slack::Thread::numCreated());
}

int main()
{
    test(0);
    test(1);
    test(5);
    test(10);
    test(50);
}