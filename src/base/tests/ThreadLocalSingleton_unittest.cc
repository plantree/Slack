/*
 * @Author: py.wang 
 * @Date: 2019-05-11 08:33:27 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-11 08:51:06
 */

#include "src/base/ThreadLocalSingleton.h"
#include "src/base/CurrentThread.h"
#include "src/base/Thread.h"

#include <stdio.h>

class Test : slack::noncopyable
{
public:
    Test()
    {
        printf("tid=%d, constructing %p\n", slack::CurrentThread::tid(), this);
    }
    ~Test()
    {
        printf("tid=%d, destructing %p %s\n", slack::CurrentThread::tid(), this, name_.c_str());
    }
    const slack::string &name() const
    {
        return name_;
    }
    void setName(const slack::string &name)
    {
        name_ = name;
    }
private:
    slack::string name_;
};

void threadFunc(const char *changeTo)
{
    printf("tid=%d, %p name=%s\n",
            slack::CurrentThread::tid(),
            &slack::ThreadLocalSingleton<Test>::instance(),
            slack::ThreadLocalSingleton<Test>::instance().name().c_str());

    slack::ThreadLocalSingleton<Test>::instance().setName(changeTo);

    printf("tid=%d, %p name=%s\n",
            slack::CurrentThread::tid(),
            &slack::ThreadLocalSingleton<Test>::instance(),
            slack::ThreadLocalSingleton<Test>::instance().name().c_str());
}

int main()
{
    slack::ThreadLocalSingleton<Test>::instance().setName("main one");
    slack::Thread t1(std::bind(threadFunc, "thread1"));
    slack::Thread t2(std::bind(threadFunc, "thread2"));
    t1.start();
    t2.start();
    t1.join();

    printf("tid=%d, %p name=%s\n",
            slack::CurrentThread::tid(),
            &slack::ThreadLocalSingleton<Test>::instance(),
            slack::ThreadLocalSingleton<Test>::instance().name().c_str());
    
    t2.join();
    pthread_exit(nullptr);
}