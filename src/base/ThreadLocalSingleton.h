/*
 * @Author: py.wang 
 * @Date: 2019-05-11 08:23:31 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-11 08:42:00
 */

#ifndef BASE_THREADLOCALSINGLETON
#define BASE_THREADLOCALSINGLETON

#include "src/base/noncopyable.h"

#include <assert.h>
#include <pthread.h>

namespace slack
{

template <typename T>
class ThreadLocalSingleton : noncopyable
{
public:
    // 禁止在栈上建立
    ThreadLocalSingleton() = delete;
    ~ThreadLocalSingleton() = delete;

    // 每个线程获取的实例都是各自私有的
    static T &instance()
    {
        if (!t_value_)
        {
            t_value_ = new T();
            deleter_.set(t_value_);
        }
        return *t_value_;
    }

    static T *pointer()
    {
        return t_value_;
    }

private:
    // 类共享删除器
    static void destructor(void *obj)
    {
        assert(static_cast<T *>(obj) == t_value_);
        delete t_value_;
        t_value_ = nullptr;
    }

    // 内部类
    class Deleter 
    {
    public:
        Deleter()
        {
            pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
        }
        ~Deleter()
        {
            pthread_key_delete(pkey_);
        }
        void set(T *newObj)
        {
            assert(pthread_getspecific(pkey_) == nullptr);
            pthread_setspecific(pkey_, newObj);
        }
        pthread_key_t pkey_;
    };
    static __thread T *t_value_;    // 线程局部数据
    static Deleter deleter_;        // 类共享删除器
};

template <typename T>
__thread T *ThreadLocalSingleton<T>::t_value_ = nullptr;

template <typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

}   // namespace slack

#endif  // BASE_THREADLOCALSINGLETON