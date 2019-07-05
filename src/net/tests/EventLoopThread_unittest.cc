/*
 * @Author: py.wang 
 * @Date: 2019-05-25 08:05:01 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-25 08:13:14
 */

#include "src/net/EventLoopThread.h"
#include "src/net/EventLoop.h"
#include "src/base/Thread.h"
#include "src/base/CountDownLatch.h"

#include <stdio.h>
#include <unistd.h>

using namespace slack;
using namespace slack::net;

void print(EventLoop *p = nullptr)
{
    printf("print: pid = %d, tid = %d, loop = %p\n",
            getpid(), CurrentThread::tid(), p);
}

void quit(EventLoop *p)
{
    print(p);
    p->quit();
}

int main()
{
    print();

    {
        EventLoopThread thr1;   // never start
    }

    {
        // dtor calls quit()
        EventLoopThread thr2;
        EventLoop *loop = thr2.startLoop();
        // 异步调用runInLoop，添加到loop对象的IO线程
        loop->runInLoop(std::bind(print, loop));
        sleep(1);
    }

    {
        // quit() before dtor
        EventLoopThread thr3;
        EventLoop *loop = thr3.startLoop();
        loop->runInLoop(std::bind(quit, loop));
        sleep(1);
    }

    
}