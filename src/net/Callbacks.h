/*
 * @Author: py.wang 
 * @Date: 2019-05-22 07:52:27 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-01 09:29:43
 */

#ifndef NET_CALLBACKS_H
#define NET_CALLBACKS_H

#include "src/base/Timestamp.h"

#include <functional>
#include <memory>

namespace slack
{

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

// should really belong to bast/Types.h but 
// <memory> is not included there.

template <typename T>
inline T *get_pointer(const std::shared_ptr<T> &ptr)
{
    return ptr.get();
}

template <typename T>
inline T *get_pointer(const std::unique_ptr<T> &ptr)
{
    return ptr.get();
}

// FIXME
// Adapted from google-probuf stubs/common.h
template <typename To, typename From>
inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From> &f)
{
    if (false)
    {
        implicit_cast<From *, To *>(0);
    }
#ifndef NDEBUG
    assert(f == nullptr || dynamic_cast<To *>(get_pointer(f)) != nullptr);
#endif
    return ::std::static_pointer_cast<To>(f);
}

namespace net
{

// Alll client visible callbacks go here

class Buffer;
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void()> TimerCallback;
typedef std::function<void(const TcpConnectionPtr &)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr &)> CloseCallback;
typedef std::function<void(const TcpConnectionPtr &)> WriteCompleteCallback;
typedef std::function<void(const TcpConnectionPtr &, size_t)> HighWaterMarkCallback;

// the data has been read to (buf, len)
typedef std::function<void(const TcpConnectionPtr &,
                            Buffer *,
                            Timestamp)> MessageCallback;

void defaultConnectionCallback(const TcpConnectionPtr &conn);
void defaultMessageCallback(const TcpConnectionPtr &conn,
                            Buffer *buffer,
                            Timestamp receiveTime);

}   // namespace net

}   // namespace slack

#endif