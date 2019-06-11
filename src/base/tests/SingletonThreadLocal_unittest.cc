/*
 * @Author: py.wang 
 * @Date: 2019-05-11 07:53:57 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-11 08:13:16
 */

#include "src/base/Singleton.h"
#include "src/base/CurrentThread.h"
#include "src/base/ThreadLocal.h"
#include "src/base/Thread.h"
#include "src/base/noncopyable.h"

#include <functional>

#include <stdio.h>
#include <unistd.h>

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
    void setName(const std::string &name)
    {
        name_ = name;
    }
private:
    slack::string name_;
};

// TODO: 同一个实例，线程局部数据
#define STL slack::Singleton<slack::ThreadLocal<Test>>::instance().value()

void print()
{
    printf("tid=%d, %p name=%s\n",
            slack::CurrentThread::tid(),
            &STL,
            STL.name().c_str());
}

void threadFunc(const char *changeTo)
{
    print();
    STL.setName(changeTo);
    sleep(1);
    print();
}

int main()
{
    STL.setName("main one");
    slack::Thread t1(std::bind(threadFunc, "thread1"));
    slack::Thread t2(std::bind(threadFunc, "thread2"));
    t1.start();
    t2.start();
    t1.join();

    print();
    t2.join();
    // 终止主线程
    pthread_exit(nullptr);
}