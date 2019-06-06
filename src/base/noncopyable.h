/*
 * @Author: py.wang 
 * @Date: 2019-05-04 08:02:05 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-07 09:09:07
 */
//#pragma once
#ifndef BASE_NONCOPYABLE_H
#define BASE_NONCOPYABLE_H

namespace slack
{
    
class noncopyable
{
public:
    // 拷贝构造函数和赋值运算符声明为删除
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

}   // namespace slack

#endif  // BASE_NONCOPYABLE_H