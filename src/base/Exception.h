/*
 * @Author: py.wang 
 * @Date: 2019-05-04 09:03:13 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-06 17:57:21
 */

// TODO noexcept

#ifndef BASE_EXCEPTION_H
#define BASE_EXCEPTION_H

#include "src/base/Types.h"

#include <exception>

namespace slack
{

class Exception : public std::exception 
{
public:
    Exception(string what);
    // 保证不抛出异常
    ~Exception() noexcept override = default;

    // default copy-ctor and assignment are ok
    
    const char *what() const noexcept override
    {
        return message_.c_str();
    }

    const char *stackTrace() const noexcept
    {
        return stack_.c_str();
    }

private:
    string message_;
    string stack_;
};

}   // namespace slack

#endif  // BASE_EXCEPTION_H