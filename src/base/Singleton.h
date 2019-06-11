/*
 * @Author: py.wang 
 * @Date: 2019-05-10 08:26:19 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-10 08:58:24
 */

#ifndef BASE_SINGLETON_H
#define BASE_SINGLETON_H

#include "src/base/noncopyable.h"

#include <pthread.h>
#include <assert.h>
#include <stdlib.h> // atexit

namespace slack
{

// FIXME: has_no_destroy

template <typename T>
class Singleton : noncopyable
{
public:
    // 不能在栈上construct
    Singleton() = delete;
    ~Singleton() = delete;
    static T &instance()
    {
        pthread_once(&ponce_, &Singleton::init);    // only init once
        assert(value_ != nullptr);
        return *value_;
    }

private:
    static void init()
    {
        value_ = new T();
        ::atexit(destroy);  // 注册析构时的行为
    }

    static void destroy()
    {
        // 不完全类型检测, no-need，new的时候就会出错
        //typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
        //T_must_be_complete_type dummy; (void)dummy;

        delete value_;
        value_ = nullptr;
    }

    static pthread_once_t ponce_;
    static T *value_;
};

template <typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

template <typename T>
T *Singleton<T>::value_ = nullptr;

}   // namespace slack

#endif  // BASE_SINGLETON_H