/*
 * @Author: py.wang 
 * @Date: 2019-06-09 16:46:10 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-09 17:02:18
 */

#include "src/base/Thread.h"
#include "src/base/CurrentThread.h"

#include <string>
#include <functional>

#include <stdio.h>
#include <unistd.h>

void mysleep(int seconds)
{
    timespec t = {seconds, 0};
    nanosleep(&t, nullptr);
}

void threadFunc()
{
    printf("tid=%d\n", slack::CurrentThread::tid());
}

void threadFunc2(int x)
{
    printf("tid=%d, x=%d\n", slack::CurrentThread::tid(), x);
}

void threadFunc3()
{
    printf("tid=%d\n", slack::CurrentThread::tid());
    mysleep(1);
}

class Foo
{
public:
    explicit Foo(double x) 
        : x_(x)
    {
    }
    void memberFunc()
    {
        printf("tid=%d, Foo::x_=%f\n", slack::CurrentThread::tid(), x_);
    }

    void memeberFunc2(const std::string &text)
    {
        printf("tid=%d, Foo::x_=%f, text=%s\n", slack::CurrentThread::tid(), x_, text.c_str());
    }
private:
    double x_;
};

int main()
{
    // main thread
    printf("pid=%d, tid=%d\n", ::getpid(), slack::CurrentThread::tid());
    printf("Main Thread: %s\n", slack::CurrentThread::isMainThread() ? "true" : "false");
    
    slack::Thread t1(threadFunc);
    t1.start();
    t1.join();

    slack::Thread t2(std::bind(threadFunc2, 42), "thread for free function with argument");
    t2.start();
    t2.join();

    Foo foo(87.53);
    slack::Thread t3(std::bind(&Foo::memberFunc, &foo), "thread for member function without argument");
    t3.start();
    t3.join();

    // 引用传递
    slack::Thread t4(std::bind(&Foo::memeberFunc2, std::ref(foo), std::string("Peng")));
    t4.start();
    t4.join();

    {
        slack::Thread t5(threadFunc3);
        t5.start();
        // t5 may destruct earlier than thread creation
    }
    mysleep(2);
    {
        slack::Thread t6(threadFunc3);
        t6.start();
        mysleep(2);
        // t6 destruct later than thread creation
    }
    sleep(2);
    printf("number of created threads %d\n", slack::Thread::numCreated());
}


