/*
 * @Author: py.wang 
 * @Date: 2019-05-10 09:21:14 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-09 19:28:02
 */

#ifndef BASE_THREADLOCAL_H
#define BASE_THREADLOCAL_H

#include "src/base/Mutex.h"
#include "src/base/noncopyable.h"

#include <pthread.h>

namespace slack
{

// key-value
template <typename T>
class ThreadLocal : noncopyable
{
public:
    ThreadLocal()
    {
        // key删除的时候value的dtor
        int ret = pthread_key_create(&pkey_, &ThreadLocal::destructor);
        assert(ret == 0); (void)ret;
    }

    ~ThreadLocal()
    {
        int ret = pthread_key_delete(pkey_);
        assert(ret == 0); (void)ret;
    }

    T &value()
    {
        T *perThreadValue = static_cast<T *>(pthread_getspecific(pkey_));
        // 键对应值不存在，新建
        if (!perThreadValue)
        {
            T *newObj = new T();
            pthread_setspecific(pkey_, newObj);
            perThreadValue = newObj;
        }
        return *perThreadValue;
    }

private:
    static void destructor(void *x)
    {
        T *obj = static_cast<T *>(x);
        delete obj;
    }
    pthread_key_t pkey_;
};
    
} // namespace slack


#endif  // BASE_THREADLOCAL_H