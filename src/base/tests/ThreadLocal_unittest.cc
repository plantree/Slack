/*
 * @Author: py.wang 
 * @Date: 2019-05-10 09:27:37 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-09 19:20:26
 */

#include "src/base/ThreadLocal.h"
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
    void setName(const std::string &n)
    {
        name_ = n;
    }
private:
    slack::string name_;
};

// constuct
slack::ThreadLocal<Test> testObj1;
slack::ThreadLocal<Test> testObj2;

void print()
{
    printf("tid=%d, obj1 %p name=%s\n",
            slack::CurrentThread::tid(),
            &testObj1.value(),
            testObj1.value().name().c_str());
    printf("tid=%d, obj2 %p name=%s\n",
            slack::CurrentThread::tid(),
            &testObj2.value(),
            testObj2.value().name().c_str());
}

void threadFunc()
{
    print();
    testObj1.value().setName("changed 1");
    testObj2.value().setName("changed 42");
    print();
}

int main()
{
    testObj1.value().setName("main one");
    print();

    // change
    slack::Thread t1(threadFunc);
    t1.start();
    t1.join();

    testObj2.value().setName("main two");
    print();

    exit(0);
}
