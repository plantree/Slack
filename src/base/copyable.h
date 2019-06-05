/*
 * @Author: py.wang 
 * @Date: 2019-05-02 08:42:44 
 * @Last Modified by:   py.wang 
 * @Last Modified time: 2019-05-02 08:42:44 
 */

#ifndef BASE_COPYABLE_H
#define BASE_COPYABLE_H

namespace slack
{

// 允许拷贝对象的父类
class copyable
{
    protected:
        // c++11语义
        copyable() = default;
        ~copyable() = default;
};

} // namespace slack

#endif // BASE_COPYABLE_H