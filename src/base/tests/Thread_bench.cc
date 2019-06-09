/*
 * @Author: py.wang 
 * @Date: 2019-06-09 17:02:51 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-09 18:20:15
 */
#include "src/base/CurrentThread.h"
#include "src/base/Mutex.h"
#include "src/base/Thread.h"
#include "src/base/Timestamp.h"

#include <map>
#include <string>

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

slack::MutexLock g_mutex;
std::map<int, int> g_delays;

void threadFunc()
{
    //printf("tid=%d\n", slack::CurrentThread::tid());
}

void threadFunc2(slack::Timestamp start)
{
    slack::Timestamp now(slack::Timestamp::now());
    int delay = static_cast<int>(slack::timeDifference(now, start) * slack::Timestamp::kMicroSecondsPerSecond);
    slack::MutexLockGuard lock(g_mutex);
    ++g_delays[delay];
}

void forkBench()
{
    sleep(10);
    slack::Timestamp start(slack::Timestamp::now());
    int kProcesses = 10 * 1000;

    for (int i = 0; i < kProcesses; ++i)
    {
        pid_t child = fork();
        if (child == 0)
        {
            _exit(0);
        }
        else 
        {
            waitpid(child, nullptr, 0);
        }
    }

    double timeUsed = slack::timeDifference(slack::Timestamp::now(), start);
    printf("per process creation time used %f us\n", timeUsed * 1000000 / kProcesses);
    printf("number of created processed %d\n", kProcesses);
}

int main(int argc, char *argv[])
{
    printf("pid=%d, tid=%d\n", ::getpid(), slack::CurrentThread::tid());
    slack::Timestamp start(slack::Timestamp::now());

    int kThreads = 100 * 1000;
    for (int i = 0; i < kThreads; ++i)
    {
        slack::Thread t1(threadFunc);
        t1.start();
        t1.join();
    }

    double timeUsed = slack::timeDifference(slack::Timestamp::now(), start);
    printf("per thread creation time used %f us\n", timeUsed * 1000000 / kThreads);
    printf("number of created threads %d\n", kThreads);

    for (int i = 0; i < kThreads; ++i)
    {
        slack::Timestamp now(slack::Timestamp::now());
        slack::Thread t2(std::bind(threadFunc2, now));
        t2.start();
        t2.join();
    }
    {
        slack::MutexLockGuard lock(g_mutex);
        for (const auto &delay : g_delays)
        {
            printf("delay=%d, count=%d\n", delay.first, delay.second);
        }
    }
    forkBench();
}