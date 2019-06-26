/*
 * @Author: py.wang 
 * @Date: 2019-05-31 09:18:31 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-31 09:24:44
 */
#include "src/net/Buffer.h"
#include "src/net/SocketsOps.h"

#include <errno.h>
#include <sys/uio.h>

using namespace slack;
using namespace slack::net;

const char Buffer::kCRLF[] = "\r\n";

// 静态变量定义
const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

// 结合栈上空间，避免内存使用过大，提高内存使用率
// 整体写好过零散写
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
    
    const int iovcnt = (writable < sizeof extraBuf) ? 2 : 1;
    const ssize_t n = sockets::readv(fd, vec, iovcnt);
    // error
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (implicit_cast<size_t>(n) <= writable)  // 第一块缓冲区足够
    {
        // 没有使用栈上空间
        writerIndex_ += n;
    }
    else    // 第一块不够， append
    {
        // update writerIndex_
        writerIndex_ = buffer_.size();
        append(extraBuf, n-writable);
    }
    return n;
}