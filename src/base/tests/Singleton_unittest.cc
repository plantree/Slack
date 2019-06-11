/*
 * @Author: py.wang 
 * @Date: 2019-05-10 08:37:49 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-10 09:36:04
 */

#include "src/base/Singleton.h"
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

void threadFunc()
{
    printf("tid=%d, %p name=%s\n",
            slack::CurrentThread::tid(),
            &slack::Singleton<Test>::instance(),
            slack::Singleton<Test>::instance().name().c_str());
    slack::Singleton<Test>::instance().setName("only one, changed");
}

class B;

int main()
{
    slack::Singleton<Test>::instance().setName("only one");
    slack::Thread t1(threadFunc);
    t1.start();
    t1.join();
    printf("tid=%d, %p name=%s\n",
            slack::CurrentThread::tid(),
            &slack::Singleton<Test>::instance(),
            slack::Singleton<Test>::instance().name().c_str());

    // incomplete type, error
    //slack::Singleton<B>::instance();
}