/*
 * @Author: py.wang 
 * @Date: 2019-05-20 08:38:50 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-24 09:14:08
 */

#include "src/net/EventLoop.h"
#include "src/base/Thread.h"
#include "src/base/CurrentThread.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

using namespace slack;
using namespace slack::net;

EventLoop *g_loop;

void callback()
{
    printf("callback(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    EventLoop anotherLoop;
}

void threadFunc()
{
    printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

    assert(EventLoop::getEventLoopOfCurrentThread() == nullptr);
    
    EventLoop loop;
    assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
    
    loop.runAfter(1.0, callback);
    loop.loop();
}

int main()
{
    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

    assert(EventLoop::getEventLoopOfCurrentThread() == nullptr);
    EventLoop loop;
    assert(EventLoop::getEventLoopOfCurrentThread() == &loop);

    Thread thread(threadFunc);
    thread.start();

    loop.loop();
}

