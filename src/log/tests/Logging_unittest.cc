/*
 * @Author: py.wang 
 * @Date: 2019-05-14 08:08:27 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-14 09:06:04
 */

#include "src/log/Logging.h"
#include "src/base/ThreadPool.h"
#include "src/base/CurrentThread.h"

#include <stdio.h>
#include <unistd.h>

int g_total;
FILE *g_file;

void dummyOutput(const char *msg, int len)
{
    g_total += len;
    if (g_file)
    {
        fwrite(msg, 1, len, g_file);
    }
    else 
    {
        fwrite(msg, 1, len, stdout);
    }
}

void bench(const char *type)
{
    slack::Logger::setOutput(dummyOutput);
    slack::Timestamp start(slack::Timestamp::now());
    g_total = 0;

    int n = 1000 * 1000;
    const bool kLongLog = false;
    slack::string empty = " ";
    slack::string longStr(3000, 'X');
    longStr += " ";
    for (int i = 0; i < n; ++i)
    {
        LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz"
                << (kLongLog ? longStr : empty) << i;
    }
    slack::Timestamp end(slack::Timestamp::now());
    double seconds = slack::timeDifference(end, start);
    printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f Mib/s ",
            type, seconds, g_total, n/seconds, g_total/seconds/(1024*1024));
}

void logInThread()
{
    LOG_INFO << "logInThread " << slack::CurrentThread::tid() << " ";
    usleep(1000);
}

int main()
{
    getppid();  // for ltrace and strace

    slack::ThreadPool pool("pool");
    pool.start(5);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);

    LOG_TRACE << "trace";
    LOG_DEBUG << "debug";
    LOG_INFO << "Hello";
    LOG_WARN << "World";
    LOG_ERROR << "Error";
    LOG_INFO << sizeof(slack::Logger);
    LOG_INFO << sizeof(slack::LogStream);
    LOG_INFO << sizeof(slack::Fmt);
    LOG_INFO << sizeof(slack::LogStream::Buffer);

    sleep(1);
    bench("nop");

    char buffer[64*1024];

    g_file = fopen("/dev/null", "w");
    setbuffer(g_file, buffer, sizeof buffer);
    bench("/dev/null");
    fclose(g_file);

    g_file = fopen("/tmp/log", "w");
    setbuffer(g_file, buffer, sizeof buffer);
    bench("/tmp/log");
    fclose(g_file);

}