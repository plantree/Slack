/*
 * @Author: py.wang 
 * @Date: 2019-06-06 18:03:51 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-09 16:58:47
 */
#include "src/base/CurrentThread.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>

namespace slack
{

namespace CurrentThread
{

__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char *t_threadName = "unknown";
// 同一类型判断
static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

string stackTrace(bool demangle)
{
    string stack;
    const int max_frames = 200; // 栈的最大深度
    void *frame[max_frames];
    // 获取调用的栈信息
    int nptrs = ::backtrace(frame, max_frames);
    // WARNING: 要自己释放
    char **strings = ::backtrace_symbols(frame, nptrs);  // translate
    if (strings)
    {
        size_t len = 256;
        char *demangled = demangle ? static_cast<char *>(::malloc(len)) : nullptr;
        for (int i = 1; i < nptrs; ++i) // skip the 0-th(this function)
        {
            if (demangled)
            {
                // bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
                char *left_par = nullptr;   // (
                char *plus = nullptr;       // +

                for (char *p = strings[i]; *p; ++p)
                {
                    if (*p == '(')
                    {
                        left_par = p;
                    }
                    else if (*p == '+')
                    {
                        plus = p;
                    }
                }

                if (left_par && plus)
                {
                    *plus = '\0';
                    // demangle
                    int status = 0;
                    char *ret = abi::__cxa_demangle(left_par+1, demangled, &len, &status);
                    *plus = '+';
                    // succeed
                    if (status == 0)
                    {
                        demangled = ret; // could be realloc
                        stack.append(strings[i], left_par+1);
                        stack.append(demangled);
                        stack.append(plus);
                        stack.push_back('\n');
                        continue;
                    }
                }
            }
             // append mangled names
            stack.append(strings[i]);
            stack.push_back('\n');
        }
        free(demangled);
        free(strings);
    }
    return stack;
}

}   // namespace CurrentThread

}   // namespace slack