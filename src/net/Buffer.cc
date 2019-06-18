/*
 * @Author: py.wang 
 * @Date: 2019-05-31 09:18:31 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-31 09:24:44
 */
#include <muduo/net/Buffer.h>
#include <muduo/net/SocketsOps.h>

#include <errno.h>
#include <sys/uio.h>

using namespace muduo;
using namespace muduo::net;

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

// 结合栈上空间，避免内存使用过大，提高内存使用率
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extraBuf[65535];
    struct iovec vec[2];
    const size_t writable = writableBytes();
    // 第一块缓冲区
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    // 第二块缓冲区
    vec[1].iov_base = extraBuf;
    vec[1].iov_len = sizeof extraBuf;
    const ssize_t n = sockets::readv(fd, vec, 2);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (implicit_cast<size_t>(n) <= writable)  // 第一块缓冲区足够
    {
        writerIndex_ += n;
    }
    else    // 第一块不够，append
    {
        writerIndex_ = buffer_.size();
        append(extraBuf, n-writable);
    }
    return n;
}