/*
 * @Author: py.wang 
 * @Date: 2019-05-25 09:26:44 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-27 08:17:46
 */

#include "src/net/SocketsOps.h"
#include "src/net/Endian.h"
#include "src/log/Logging.h"
#include "src/base/Types.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>      // snprintf
#include <strings.h>
#include <sys/socket.h>
#include <sys/uio.h>    // 聚集读、分散写
#include <unistd.h>

using namespace slack;
using namespace slack::net;

// 匿名命名空间
namespace 
{

typedef struct sockaddr SA;

#if VALGRIND || defined (NO_ACCEPT4)
void setNonBlockingAndCloseOnExec(int sockfd)
{
    // non-block
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);

    // close-on-exec
    flgas = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);
    (void)ret;
}
#endif

}   // namespace 

const struct sockaddr *sockets::sockaddr_cast(const struct sockaddr_in6 *addr) 
{
    return static_cast<const struct sockaddr *>(implicit_cast<const void *>(addr)); 
}

struct sockaddr *sockets::sockaddr_cast(struct sockaddr_in6 *addr)
{
    return static_cast<struct sockaddr *>(implicit_cast<void *>(addr));
}

const struct sockaddr *sockets::sockaddr_cast(const struct sockaddr_in *addr)
{
    return static_cast<const struct sockaddr *>(implicit_cast<const void *>(addr));
}

const struct sockaddr_in *sockets::sockaddr_in_cast(const struct sockaddr *addr)
{
    return static_cast<const struct sockaddr_in *>(implicit_cast<const void *>(addr));
}

const struct sockaddr_in6 *sockets::sockaddr_in6_cast(const sockaddr *addr)
{
    return static_cast<const struct sockaddr_in6 *>(implicit_cast<const void *>(addr));
}

int sockets::createNonBlockingOrDie(sa_family_t family)
{
    // socket
#if VALGRIND
    int sockfd = ::socket(family, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        LOG_SYSFATAL << "sockets::createNonBlockingOrDie";
    }
    setBlockingAndCloseOnExec(sockfd);
#else
    // Linux 2.6.27以上内核支持SOCK_NONBLOCK与SOCK_CLOEXEC
    // 直接就是非阻塞和执行时关闭
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0)
    {
        LOG_SYSFATAL << "sockets::createNonBlockingOrDie";
    }
#endif
    return sockfd;
}

/*
void setBlockingAndCloseOnExec(int sockfd) __attribute__((unused));

void setBlockingAndCloseOnExec(int sockfd)
{
    // non-block
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    // 先获取再修改，避免清除之前的信息
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);
    // FIXME check
    if (ret == -1)
    {
        LOG_SYSFATAL << "fcntl set O_NONBLOCK";
    }

    // close-on-exec
    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);
    if (ret == -1)
    {
        LOG_SYSFATAL << "fcntl set FD_CLOEXEC";
    }
    (void)ret;
}*/

void sockets::bindOrDie(int sockfd, const struct sockaddr *addr)
{
    int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if (ret < 0)
    {
        LOG_SYSFATAL << "sockets::bindOrDie";
    }
}

void sockets::listenOrDie(int sockfd)
{
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0)
    {
        LOG_SYSFATAL << "sockets::listenOrDie";
    }
}

int sockets::accept(int sockfd, struct sockaddr_in6 *addr)
{
    // 所有连接套接字都是非阻塞
    socklen_t addrlen = sizeof *addr;
#if VALGRIND || defined (NO_ACCEPT4)
    int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
    setBlockingAndCloseOnExec(connfd);
#else
    // non-blocking, close-on-exec connection
    int connfd = ::accept4(sockfd, sockaddr_cast(addr),
                            &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
    if (connfd < 0)
    {
        int savedErrno = errno;
        LOG_SYSERR << "sockets::accept";
        switch (savedErrno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:    // 协议错误
            case EPERM: // 防火墙阻止
            case EMFILE: // per-process limit on the number of open file descriptors
                // expected error
                errno = savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors
                LOG_FATAL << "unexpected error of ::accept " << savedErrno;
                break;
            default:
                LOG_FATAL << "unknown error of ::accept " << savedErrno;
                break;
        }
    }
    return connfd;
}

int sockets::connect(int sockfd, const struct sockaddr *addr)
{
    return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

ssize_t sockets::read(int sockfd, void *buf, size_t count)
{
    return ::read(sockfd, buf, count);
}

// readv是分散读，接收到的数据被放入多个缓冲区
ssize_t sockets::readv(int sockfd, const struct iovec *iov, int iovcnt)
{
    // #include <sys/uio.h>
    return ::readv(sockfd, iov, iovcnt);
}

ssize_t sockets::write(int sockfd, const void *buf, size_t count)
{
    return ::write(sockfd, buf, count);
}

void sockets::close(int sockfd)
{
    if (::close(sockfd) < 0)
    {
        LOG_SYSERR << "sockets::close";
    }
}

// 只关闭写
void sockets::shutDownWrite(int sockfd)
{
    if (::shutdown(sockfd, SHUT_WR) < 0)
    {
        LOG_SYSERR << "sockets::shutdownWrite";
    }
}

// 地址转换为ip:port字符串
void sockets::toIpPort(char *buf, size_t size,
                        const struct sockaddr *addr)
{
    toIp(buf, size, addr);
    size_t end = ::strlen(buf);
    const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
    uint16_t port = sockets::networkToHost16(addr4->sin_port);
    assert(size > end);
    snprintf(buf+end, size-end, ":%u", port);   
}

// 地址转换为ip字符串
void sockets::toIp(char *buf, size_t size, 
                    const struct sockaddr *addr)
{
    if (addr->sa_family == AF_INET)
    {
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
        ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
    }
    else if (addr->sa_family == AF_INET6)
    {
        assert(size >= INET6_ADDRSTRLEN);
        const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
        ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
    }
}

void sockets::fromIpPort(const char *ip, uint16_t port,
                        struct sockaddr_in *addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port = hostToNetwork16(port);
    if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
    {
        LOG_SYSERR << "sockets::fromIpPort";
    }
}

void sockets::fromIpPort(const char *ip, uint16_t port, 
                        struct sockaddr_in6 *addr)
{
    addr->sin6_family = AF_INET6;
    addr->sin6_port = hostToNetwork16(port);
    if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0)
    {
        LOG_SYSERR << "sockets::fromIpPort";
    }
}

int sockets::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else 
    {
        return optval;
    }
}

struct sockaddr_in6 sockets::getLocalAddr(int sockfd)
{
    struct sockaddr_in6 localAddr;
    memZero(&localAddr, sizeof localAddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localAddr);
    if (::getsockname(sockfd, sockaddr_cast(&localAddr), &addrlen) < 0)
    {
        LOG_SYSERR << "sockets::getLocalAddr";
    }
    return localAddr;
}

struct sockaddr_in6 sockets::getPeerAddr(int sockfd)
{
    struct sockaddr_in6 peerAddr;
    memZero(&peerAddr, sizeof peerAddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peerAddr);
    if (::getpeername(sockfd, sockaddr_cast(&peerAddr), &addrlen) < 0)
    {
        LOG_SYSERR << "sockets::getPeerAddr";
    }
    return peerAddr;
}

// 自连接是(sourceIp, sourcePort) = (destIp, destPort)
// 客户端在connect的时候没有bind(2)
// 客户端与服务端在一个机器
// 服务尚未开启
// FIXME
bool sockets::isSelfConnect(int sockfd)
{
    struct sockaddr_in6 localAddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peerAddr = getPeerAddr(sockfd);
    if (localAddr.sin6_family == AF_INET)
    {
        // 重新解释
        const struct sockaddr_in *laddr4 = reinterpret_cast<struct sockaddr_in *>(&localAddr);
        const struct sockaddr_in *raddr4 = reinterpret_cast<struct sockaddr_in *>(&peerAddr);
        return laddr4->sin_port == raddr4->sin_port 
            && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
    }
    else if (localAddr.sin6_family == AF_INET6)
    {
        // 按位比较
        return localAddr.sin6_port == peerAddr.sin6_port
            && memcmp(&localAddr.sin6_addr, &peerAddr.sin6_addr, sizeof localAddr.sin6_addr) == 0;
    }
    else 
    {
        return false;
    }
}
