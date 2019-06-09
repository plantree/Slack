/*
 * @Author: py.wang 
 * @Date: 2019-05-07 09:11:32 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-09 18:26:51
 */

#include "src/base/Mutex.h"
#include "src/base/Thread.h"
#include "src/base/Timestamp.h"
#include "src/base/CountDownLatch.h"

#include <vector>
#include <stdio.h>

using namespace std;
using namespace slack;

MutexLock g_mutex;
vector<int> g_vec;
const int kCount = 10*1000*1000;

void threadFunc()
{
    for (int i = 0; i < kCount; ++i)
    {
        MutexLockGuard lock(g_mutex);
        g_vec.push_back(i);
    }
}

int main()
{
    const int kMaxThreads = 8;
    g_vec.reserve(kMaxThreads * kCount);

    Timestamp start(Timestamp::now());
    for (int i = 0; i < kCount; ++i)
    {
        g_vec.push_back(i);
    }

    printf("single thread without lock %f\n", timeDifference(Timestamp::now(), start));

    start = Timestamp::now();
    threadFunc();
    printf("single thread with lock %f\n", timeDifference(Timestamp::now(), start));

    for (int nthreads = 1; nthreads < kMaxThreads; ++nthreads)
    {
        // unique_ptr负责管理资源
        vector<unique_ptr<Thread>> threads;
        g_vec.clear();
        start = Timestamp::now();
        // add thread and work
        for (int i = 0; i < nthreads; ++i)
        {
            threads.emplace_back(new Thread(&threadFunc));
            threads.back()->start();
        }

        for (int i = 0; i < nthreads; ++i)
        {
            threads[i]->join();
        }
        printf("%d thread(s) with lock %f\n", nthreads, timeDifference(Timestamp::now(), start));
    }
}