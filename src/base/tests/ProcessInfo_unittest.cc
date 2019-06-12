/*
 * @Author: py.wang 
 * @Date: 2019-05-15 09:17:27 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-15 09:25:22
 */

#include "src/base/ProcessInfo.h"
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

int main()
{
    printf("pid = %d\n", slack::ProcessInfo::pid());
    printf("uid = %d\n", slack::ProcessInfo::uid());
    printf("euid = %d\n", slack::ProcessInfo::euid());
    printf("start time = %s\n", slack::ProcessInfo::startTime().toFormattedString().c_str());
    printf("hostname = %s\n", slack::ProcessInfo::hostName().c_str());
    printf("opened files = %d\n", slack::ProcessInfo::openedFiles());
    printf("max open files = %d\n", slack::ProcessInfo::maxOpenFiles());
    printf("threads = %zd\n", slack::ProcessInfo::threads().size());
    printf("num threads = %d\n", slack::ProcessInfo::numThreads());
    printf("status = %s\n", slack::ProcessInfo::procStatus().c_str());
    printf("exe path = %s\n", slack::ProcessInfo::exePath().c_str());
    printf("thread stat = %s\n", slack::ProcessInfo::threadStat().c_str());
    auto t = slack::ProcessInfo::cpuTime();
    printf("cpu utime = %f, stime = %f\n", t.userSeconds, t.systemSeconds);
}