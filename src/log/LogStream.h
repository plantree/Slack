/*
 * @Author: py.wang 
 * @Date: 2019-05-12 08:33:02 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-13 08:36:31
 */

#ifndef LOG_LOGSTREAM_H
#define LOG_LOGSTREAM_H

#include "src/base/noncopyable.h"
#include "src/base/StringPiece.h"
#include "src/base/Types.h"

#include <assert.h>
#include <string.h> // memcpy

namespace slack
{

namespace detail
{

constexpr int kSmallBuffer = 4000;
constexpr int kLargeBuffer = 4000 * 1000;

template <int SIZE>
class FixedBuffer : noncopyable
{
public:
    FixedBuffer() 
        : cur_(data_) 
    {
        setCookie(cookieStart);
    }
    ~FixedBuffer()
    {
        setCookie(cookieEnd);
    }

    void append(const char /*restrict*/ *buf, size_t len)
    {
        // FIXME: append partially
        if (implicit_cast<size_t>(avail()) > len)
        {
            memcpy(cur_, buf, len); // copy
            cur_ += len;
        }
    }

    const char *data() const
    {
        return data_;
    }
    int length() const
    {
        return static_cast<int>(cur_ - data_);
    }

    // write to data_ directly
    char *current()
    {
        return cur_;
    }
    int avail() const
    {
        return static_cast<int>(end() - cur_);
    }
    void add(size_t len)
    {
        cur_ += len;
    }

    void reset()
    {
        cur_ = data_;
    }
    // 初始化为0
    void bzero()
    {
        memZero(data_, sizeof data_);
    }

    // for used by GDB
    const char *debugString();
    void setCookie(void (*cookie)())
    {
        cookie_ = cookie;
    }
    // for used by unit test
    string toString() const
    {
        return string(data_, length());
    }
    StringPiece toStringPiece() const 
    {
        return StringPiece(data_, length());
    }

private:
    const char *end() const
    {
        return data_ + sizeof data_;
    }
    // must be outline function for cookies
    // 外部设置标识
    static void cookieStart();
    static void cookieEnd();
    
    // 函数指针
    void (*cookie_)();  
    // 内部缓冲区
    char data_[SIZE];
    // 当前指针
    char *cur_;
};

}   // namespace detail

class LogStream : noncopyable
{
    using self = LogStream;
public:
    using Buffer = detail::FixedBuffer<detail::kSmallBuffer>;
    // <<运算符重载
    self &operator<<(bool v)
    {
        buffer_.append(v ? "1" : "0", 1);
        return *this;  
    }
    self &operator<<(short);
    self &operator<<(unsigned short);
    self &operator<<(int);
    self &operator<<(unsigned int);
    self &operator<<(long);
    self &operator<<(unsigned long);
    self &operator<<(long long);
    self &operator<<(unsigned long long);

    self &operator<<(const void *);

    self &operator<<(double);
    self &operator<<(float v)
    {
        *this << static_cast<double>(v);
        return *this;
    }

    self &operator<<(char v)
    {
        buffer_.append(&v, 1);
        return *this;
    }
    self &operator<<(const char *v)
    {
        if (v)
        {
            buffer_.append(v, strlen(v));
        }
        else 
        {
            buffer_.append("(null)", 6);
        }
        return *this;
    }
    self &operator<<(const string &v)
    {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }

    self &operator<<(const StringPiece &v)
    {
        buffer_.append(v.data(), v.size());
        return *this;
    }

    self &operator<<(const Buffer &v)
    {
        *this << v.toStringPiece();
        return *this;
    }

    void append(const char *data, int len)
    {
        buffer_.append(data, len);
    }
    const Buffer &buffer() const
    {
        return buffer_;
    }
    void resetBuffer(){
        buffer_.reset();
    }
private:
    void staticCheck();

    template <typename T>
    void formatInteger(T);

    Buffer buffer_;     // 内嵌FixedBuffer
    static constexpr int kMaxNumericSize = 32;
};

// 辅助format类
// 借助snprintf
class Fmt : noncopyable
{
public:
    template <typename T>
    Fmt(const char *fmt, T val);

    const char *data() const 
    {
        return buf_;
    }
    int length() const
    {
        return length_;
    }
private:
    char buf_[32];
    int length_;
};

inline LogStream &operator<<(LogStream &s, const Fmt &fmt)
{
    s.append(fmt.data(), fmt.length());
    return s;
}

}   // namespace slack

#endif  // LOG_LOGSTREAM_H