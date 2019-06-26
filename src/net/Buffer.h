/*
 * @Author: py.wang 
 * @Date: 2019-05-31 08:19:12 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-04 08:11:04
 */
#ifndef NET_BUFFER_H
#define NET_BUFFER_H

#include "src/base/copyable.h"
#include "src/base/StringPiece.h"
#include "src/base/Types.h"
#include "src/net/Endian.h"

#include <algorithm>
#include <vector>

#include <assert.h>
#include <string.h>

namespace slack
{

namespace net
{

// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
// +-------------------+------------------+------------------+
// | prependable bytes |  readable bytes  |  writable bytes  |
// |                   |     (CONTENT)    |                  |
// +-------------------+------------------+------------------+
// |                   |                  |                  |
// 0      <=      readerIndex   <=   writerIndex    <=     size
class Buffer : public copyable
{
public:
    // prependable bytes
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
        readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend)
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    // default copy-ctor, dtor and assignment are fine
    
    void swap(Buffer &rhs)
    {
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

    size_t readableBytes() const 
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const 
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const 
    {
        return readerIndex_;
    }

    // 定位到读起点
    const char *peek() const 
    {
        return begin() + readerIndex_;
    }

    // 寻找换行符 \r\n
    const char *findCRLF() const 
    {
        const char *crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    // 给定起始位置寻找换行符
    const char *findCRLF(const char *start) const 
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const char *crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    const char *findEOL() const 
    {
        const void *eol = memchr(peek(), '\n', readableBytes());
        return static_cast<const char *>(eol);
    }

    const char *findEOL(const char *start) const 
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const void *eol = memchr(start, '\n', beginWrite()-start);
        return static_cast<const char *>(eol);
    }

    // retrieve returns void, to prevent
    // string str(retrieve(readableBytes()), readableBytes())
    // the evaluation of two functions are unspecified
    
    // 索取字符串，但是
    // 仅仅只是移动readIndex
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        if (len < readableBytes())
        {
            readerIndex_ += len;
        }
        else 
        {
            retrieveAll();
        }
    }

    void retrieveUntil(const char *end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    void retrieveInt64()
    {
        retrieve(sizeof(int64_t));
    }

    void retrieveInt32()
    {
        retrieve(sizeof(int32_t));
    }

    void retrieveInt16()
    {
        retrieve(sizeof(int16_t));
    }

    void retrieveInt8()
    {
        retrieve(sizeof(int8_t));
    }

    void retrieveAll()
    {
        // reset
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // 真正索取字符串
    string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }
    
    // 真正的获取string
    string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        string result(peek(), len);
        retrieve(len);
        return result;
    }

    StringPiece toStringPiece() const 
    {
        return StringPiece(peek(), static_cast<int>(readableBytes()));
    }


    void append(const StringPiece &str)
    {
        append(str.data(), str.size());
    }

    void append(const char *data, size_t len)
    {
        // 确保有足够空间，不够的话就重新分配空间
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        // 更新writerIndex_
        hasWritten(len);
    }

    void append(const void *data, size_t len)
    {
        append(static_cast<const char *>(data), len);
    }

    // 确保可写空间，否则自动扩展
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    char *beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char *beginWrite() const 
    {
        return begin() + writerIndex_;
    }

    // index更新
    void hasWritten(size_t len)
    {
        writerIndex_ += len;
    }
    
    // 回退
    void unwrite(size_t len)
    {
        assert(len <= readableBytes());
        writerIndex_ -= len;
    }

    // append int64_t using network endian
    void appendInt64(int64_t x)
    {
        int64_t be64 = sockets::hostToNetwork64(x);
        append(&be64, sizeof be64);
    }

    void appendInt32(int32_t x)
    {
        int32_t be32 = sockets::hostToNetwork32(x);
        append(&be32, sizeof be32);
    }

    void appendInt16(int16_t x)
    {
        int16_t be16 = sockets::hostToNetwork16(x);
        append(&be16, sizeof be16);
    }
    
    void appendInt8(int8_t x)
    {
        append(&x, sizeof x);
    }

    // read int64_t from network endian
    int64_t readInt64()
    {
        int64_t result = peekInt64();
        retrieveInt64();
        return result;
    }

    int32_t readInt32()
    {
        int32_t result = peekInt32();
        retrieveInt32();
        return result;
    }

    int16_t readInt16()
    {
        int16_t result = peekInt16();
        retrieveInt16();
        return result;
    }

    int8_t readInt8()
    {
        int8_t result = peekInt8();
        retrieveInt8();
        return result;
    }

    // peek int64_t from network endian
    int64_t peekInt64() const 
    {
        assert(readableBytes() >= sizeof(int64_t));
        int64_t be64 = 0;
        ::memcpy(&be64, peek(), sizeof be64);
        return sockets::networkToHost64(be64);
    }

    int32_t peekInt32() const 
    {
        assert(readableBytes() >= sizeof(int32_t));
        int32_t be32 = 0;
        ::memcpy(&be32, peek(), sizeof be32);
        return sockets::networkToHost32(be32);
    }

    int16_t peekInt16() const 
    {
        assert(readableBytes() >= sizeof(int16_t));
        int16_t be16 = 0;
        ::memcpy(&be16, peek(), sizeof be16);
        return sockets::networkToHost16(be16);
    }

    int8_t peekInt8() const 
    {
        assert(readableBytes() >= sizeof(int8_t));
        int8_t x = *peek();
        return x;
    }

    // prepend int64_t using network endian
    void prependInt64(int64_t x)
    {
        int64_t be64 = sockets::hostToNetwork64(x);
        prepend(&be64, sizeof be64);
    }

    void prependInt32(int32_t x)
    {
        int32_t be32 = sockets::hostToNetwork32(x);
        prepend(&be32, sizeof be32);
    }

    void prependInt16(int16_t x)
    {
        int16_t be16 = sockets::hostToNetwork16(x);
        prepend(&be16, sizeof be16);
    }

    void prependInt8(int8_t x)
    {
        prepend(&x, sizeof x);
    }

    void prepend(const void *data, size_t len)
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char *d = static_cast<const char *>(data);
        std::copy(d, d+len, begin()+readerIndex_);
    }

    // 收缩，保留reserve个字节
    void shrink(size_t reserve)
    {
        // FIXME: use vector::shrink_to_fit() if possible
        Buffer other;   // empty
        // 只保留可读和预留空间
        other.ensureWritableBytes(readableBytes()+reserve);
        other.append(toStringPiece());
        swap(other);
    }

    size_t internalCapacity() const 
    {
        return buffer_.capacity();
    }

    // read data directly into buffer
    // It may implement with readv(2)
    ssize_t readFd(int fd, int *saveErrno);


private:
    char *begin()
    {
        return &(*buffer_.begin());
    }

    const char *begin() const 
    {
        return &(*buffer_.begin());
    }

    void makeSpace(size_t len)
    {
        // 空间不够大，resize
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            // FIXME: move readable data
            buffer_.resize(writerIndex_ + len);
        }
        // 空间够，但是不连续，手动碎片化整理
        else
        {
            // move readable data to the front, make space inside buffer
            // 内部腾挪
            assert(kCheapPrepend < readerIndex_);
            size_t readable = readableBytes();
            std::copy(begin()+readerIndex_,
                        begin()+writerIndex_,
                        begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());
        }
    }

    std::vector<char>buffer_;  // vector可动态扩张
    size_t readerIndex_;    // 读位置
    size_t writerIndex_;    // 写位置

    static const char kCRLF[];  // "\r\n"
};


}   // namespace net

}   // namespace slack

#endif  // NET_BUFFER
