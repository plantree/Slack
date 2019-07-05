/*
 * @Author: py.wang 
 * @Date: 2019-05-30 09:10:55 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-30 09:16:52
 */
#include "src/net/EventLoopThreadPool.h"
#include "src/net/EventLoop.h"
#include "src/base/Thread.h"
#include "src/base/CurrentThread.h"

#include <stdio.h>
#include <unistd.h>

using namespace slack;
using namespace slack::net;

void print(EventLoop *p = nullptr)
{
    printf("main(): pid = %d, tid = %d, loop = %p\n",
            getpid(), CurrentThread::tid(), p);
}

void init(EventLoop *p)
{
    printf("init(): pid = %d, tid = %d, loop = %p\n",
            getpid(), CurrentThread::tid(), p);
}

int main()
{
    print();

    EventLoop loop;
    loop.runAfter(11, std::bind(&EventLoop::quit, &loop));

    {
        printf("Single thread %p:\n", &loop);
        EventLoopThreadPool model(&loop, "single");
        model.setThreadNum(0);
        model.start(init);
        assert(model.getNextLoop() == &loop);
        assert(model.getNextLoop() == &loop);
        assert(model.getNextLoop() == &loop);
    }

    {
        printf("Another thread %p:\n", &loop);
        EventLoopThreadPool model(&loop, "another");
        model.setThreadNum(1);
        model.start(init);
        EventLoop *nextLoop = model.getNextLoop();
        nextLoop->runAfter(2, std::bind(print, nextLoop));
        assert(nextLoop != &loop);
        assert(model.getNextLoop() == nextLoop);
        assert(model.getNextLoop() == nextLoop);
        ::sleep(3);
    }

    {
        printf("Three threads %p:\n", &loop);
        EventLoopThreadPool model(&loop, "three");
        model.setThreadNum(3);
        model.start(init);
        EventLoop *nextLoop = model.getNextLoop();
        nextLoop->runInLoop(std::bind(print, nextLoop));
        assert(nextLoop != &loop);
        assert(nextLoop != model.getNextLoop());
        assert(nextLoop != model.getNextLoop());
        assert(nextLoop == model.getNextLoop());
    }

    loop.loop();
}