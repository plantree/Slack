/*
 * @Author: py.wang 
 * @Date: 2019-07-07 09:53:59 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-07 10:05:03
 */
#ifndef BASE_WEAKCALLBACK_H
#define BASE_WEAKCALLBACK_H

#include <functional>
#include <memory>

namespace slack
{

// A barely usable WeakCallback
template <typename CLASS, typename... ARGS>
class WeakCallback
{
public:
    WeakCallback(const std::weak_ptr<CLASS> &object,
                const std::function<void (CLASS *, ARGS...)> &function)
        : object_(object), function_(function)
    {
    }

    // default dtor, copy ctor and assignment are okay

    void operator()(ARGS&&... args) const 
    {
        std::shared_ptr<CLASS> ptr(object_.lock());
        if (ptr)
        {
            function_(ptr.get(), std::forward<ARGS>(args)...);
        }
    }

private:
    std::weak_ptr<CLASS> object_;
    std::function<void (CLASS *, ARGS...)> function_;
};

template <typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(const std::shared_ptr<CLASS> &object,
                                                void (CLASS::*function)(ARGS...))
{
    return WeakCallback<CLASS, ARGS...>(object, function);
}

template <typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(const std::shared_ptr<CLASS> &object,
                                                void (CLASS::*function)(ARGS...) const)
{
    return WeakCallback<CLASS, ARGS...>(object, function);
}

}   // namespace slack

#endif  // BASE_WE