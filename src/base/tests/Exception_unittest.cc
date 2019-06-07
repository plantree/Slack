/*
 * @Author: py.wang 
 * @Date: 2019-06-06 17:19:02 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-06 18:34:59
 */

#include "src/base/Exception.h"
#include "src/base/CurrentThread.h"

#include <vector>
#include <string>
#include <functional>

#include <stdio.h>

class Bar
{
public:
    void test(std::vector<std::string> names = {})
    {
        printf("Stack: \n%s\n", slack::CurrentThread::stackTrace(true).c_str());
        // lambda
        [] {
            printf("Stack inside lambda: \n%s\n", slack::CurrentThread::stackTrace(true).c_str());
        } ();
        // function
        std::function<void ()> func([] {
            printf("Stack inside function: \n%s\n", slack::CurrentThread::stackTrace(true).c_str());
        });
        func();

        func = std::bind(&Bar::callback, this);
        func();

        throw slack::Exception("oops");
    }

private:
    void callback()
    {
        printf("Stack inside std::bind: \n%s\n", slack::CurrentThread::stackTrace(true).c_str());
    }
};

void foo()
{
    Bar b;
    b.test();
}

int main()
{
    try
    {
        foo();
    }
    catch (const slack::Exception &ex)
    {
        printf("reason: %s\n", ex.what());
        printf("stack trace: \n%s\n", ex.stackTrace());
    }
}
