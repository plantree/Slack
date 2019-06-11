/*
 * @Author: py.wang 
 * @Date: 2019-05-12 08:33:02 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-13 08:36:31
 */

#ifndef MUDUO_BASE_LOGSTREAM_H
#define MUDUO_BASE_LOGSTREAM_H

#include <muduo/base/noncopyable.h>
#include <muduo/base/StringPiece.h>
#include <muduo/base/Types.h>
#include <assert.h>
#include <string.h> // memcpy
#ifndef MUDUO_STD_STRING
#include <string>
#endif

namespace muduo
{

namespace detail
{

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

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
    void bzero()
    {
        ::bzero(data_, sizeof data_);
    }

    // for used by GDB
    const char *debugString();
    void setCookie(void (*cookie)())
    {
        cookie_ = cookie;
    }
    // for used by unit test
    string asString() const
    {
        return string(data_, length());
    }

private:
    const char *end() const
    {
        return data_ + sizeof data_;
    }
    // must be outline function for cookies
    static void cookieStart();
    static void cookieEnd();
    
    void (*cookie_)();  // 函数指针
    char data_[SIZE];
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
        return *this;   // 链式
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
        buffer_.append(v, strlen(v));
        return *this;
    }
    self &operator<<(const string &v)
    {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }

//#ifndef MUDUO_STD_STRING
  //  self &operator<<(const std::string &v)
    //{
      //  buffer_.append(v.c_str(), v.size());
        //return *this;
    //}
//#endif

    self &operator<<(const StringPiece &v)
    {
        buffer_.append(v.data(), v.size());
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

// 辅助format
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

}   // namespace muduo

#endif  // MUDUO_BASE_LOGSTREAM_H