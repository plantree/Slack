/*
 * @Author: py.wang 
 * @Date: 2019-05-05 09:29:20 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-06 18:06:17
 */

#ifndef BASE_CURRENTTHREAD_H
#define BASE_CURRENTTHREAD_H

#include "src/base/Types.h"

/* CurrentThread是一个命名空间 */
namespace slack
{

// CurrentThread声明
namespace CurrentThread
{
    // internal
    // __thread局部线程基础设施，每个线程有一份独立实体
    // FIXME
    extern __thread int t_cachedTid;
    extern __thread char t_tidString[32];   // to_String(tid)
    extern __thread int t_tidStringLength;
    extern __thread const char *t_threadName;   // name
    void cachedTid();

    // 只有在调用的时候才会缓存
    inline int tid()
    {
        if (t_cachedTid == 0)
        {
            cachedTid();
        }
        return t_cachedTid;
    }

    inline const char *tidString() // for logging
    {
        return t_tidString;
    }

    inline int tidStringLength()    // for logging
    {
        return t_tidStringLength;   
    }

    inline const char *name()
    {
        return t_threadName;
    }

    bool isMainThread();

    void sleepUsec(int64_t usec);   // for testing

    string stackTrace(bool demangle);   // for exception
    
}   // namespace CurrentThread

}   // namespace slack

#endif  // BASE_CURRENTTHREAD_H
