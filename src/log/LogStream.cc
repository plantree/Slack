/*
 * @Author: py.wang 
 * @Date: 2019-05-12 09:06:38 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-14 08:36:39
 */

#include <muduo/base/LogStream.h>

#include <algorithm>
#include <limits>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::detail;

// 忽略警告
#pragma GCC diagnostic ignored "-Wtype-limits"

namespace muduo
{

namespace detail
{

const char digits[] = "9876543210123456789";
const char *zero = digits + 9;
static_assert(sizeof digits == 20);

const char digitsHex[] = "0123456789ABCDEF";
static_assert(sizeof digitsHex == 17);

// Efficient Integer to String Conversions, by Matthew Wilson
template <typename T>
size_t convert(char buf[], T value)
{
    T i = value;
    char *p = buf;

    do
    {
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd];
    } while (i != 0);
    
    if (value < 0)
    {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}

// uintptr_t->unsigned long 
size_t convertHex(char buf[], uintptr_t value)
{
    uintptr_t i = value;
    char *p = buf;

    do
    {
        int lsd = i % 16;
        i /= 16;
        *p++ = digitsHex[lsd];
    } while (i != 0);
    
    *p = '\0';
    std::reverse(buf, p);
    return p - buf;
}

} // namespace muduo

} // namespace muduo

template <int SIZE>
const char *FixedBuffer<SIZE>::debugString()
{
    *cur_ = '\0';
    return data_;
}

template <int SIZE>
void FixedBuffer<SIZE>::cookieStart()
{
}

template <int SIZE>
void FixedBuffer<SIZE>::cookieEnd()
{
}

// FIXME: 特例化
template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

// FIXME: 有点不懂
void LogStream::staticCheck()
{
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10);
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10);
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10);
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10);
}

template <typename T>
void LogStream::formatInteger(T v)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        size_t len = convert(buffer_.current(), v);
        buffer_.add(len);
    }
}

LogStream &LogStream::operator<<(short v)
{
    *this << static_cast<int>(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned short v)
{
    *this << static_cast<unsigned short>(v);
    return *this;
}

LogStream &LogStream::operator<<(int v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned int v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(long v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned long v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(long long v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned long long v)
{
    formatInteger(v);
    return *this;
}

// 转换成16进制
LogStream &LogStream::operator<<(const void *p)
{
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    if (buffer_.avail() >= kMaxNumericSize)
    {
        char *buf = buffer_.current();
        buf[0] = '0';
        buf[1] = 'x';
        size_t len = convertHex(buf+2, v);
        buffer_.add(len+2);
    }
    return *this;
}

LogStream &LogStream::operator<<(double v)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
        buffer_.add(len);
    }
    return *this;
}

template <typename T>
Fmt::Fmt(const char *fmt, T val)
{
    // 算术类型检查
    static_assert(std::is_arithmetic<T>::value == true);

    length_ = snprintf(buf_, sizeof buf_, fmt, val);
    assert(static_cast<size_t>(length_) < sizeof buf_);
}

// Explicit instantiations

template Fmt::Fmt(const char *fmt, char);
template Fmt::Fmt(const char *fmt, short);
template Fmt::Fmt(const char *fmt, unsigned short);
template Fmt::Fmt(const char *fmt, int);
template Fmt::Fmt(const char *fmt, unsigned int);
template Fmt::Fmt(const char *fmt, long);
template Fmt::Fmt(const char *fmt, unsigned long);
template Fmt::Fmt(const char *fmt, long long);
template Fmt::Fmt(const char *fmt, unsigned long long);

template Fmt::Fmt(const char *fmt, float);
template Fmt::Fmt(const char *fmt, double);






