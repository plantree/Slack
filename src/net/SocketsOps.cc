/*
 * @Author: py.wang 
 * @Date: 2019-05-25 09:26:44 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-27 08:17:46
 */

#include <muduo/net/SocketsOps.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Types.h>
#include <muduo/net/Endian.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;


namespace 
{

typedef struct  sockaddr SA;

const SA *sockaddr_cast(const struct sockaddr_in *addr) 
{
    return static_cast<const SA *>(implicit_cast<const void *>(addr)); 
}

SA *sockaddr_cast(struct sockaddr_in *addr)
{
    return static_cast<SA *>(implicit_cast<void *>(addr));
}

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
}

}

int sockets::createNonBlockingOrDie()
{
    // socket
#if VALGRIND
    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        LOG_SYSFATAL << "sockets::createNonBlockingOrDie";
    }
    setBlockingAndCloseOnExec(sockfd);
#else
    // Linux 2.6.7以上内核支持SOCK_NONBLOCK与SOCK_CLOEXEC
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0)
    {
        LOG_SYSFATAL << "sockets::createNonBlockingOrDie";
    }
#endif
    return sockfd;
}

void sockets::bindOrDie(int sockfd, const struct sockaddr_in &addr)
{
    int ret = ::bind(sockfd, sockaddr_cast(&addr), sizeof addr);
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

int sockets::accept(int sockfd, struct sockaddr_in *addr)
{
    // 所有连接套接字都是非阻塞
    socklen_t addrlen = sizeof *addr;
#if VALGRIND
    int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
    setBlockingAndCloseOnExec(connfd);
#else
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
        case EPROTO:    // 协议错误
        case EPERM: // 防火墙阻止
        case EMFILE: // per-process limit on the number of opern file descriptors
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
            LOG_FATAL << "unexptected error of ::accept " << savedErrno;
            break;
        default:
            LOG_FATAL << "unknown error of ::accept " << savedErrno;
            break;
        }
    }
    return connfd;
}

int sockets::connect(int sockfd, const struct sockaddr_in &addr)
{
    return ::connect(sockfd, sockaddr_cast(&addr), sizeof addr);
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

// 地址转换为ip字符串
void sockets::toIp(char *buf, size_t size, 
                    const struct sockaddr_in &addr)
{
    assert(size >= INET_ADDRSTRLEN);
    ::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(size));
}

// 地址转换为ip:port字符串
void sockets::toIpPort(char *buf, size_t size,
                        const struct sockaddr_in &addr)
{
    char host[INET_ADDRSTRLEN] = "INVALID";
    toIp(host, sizeof host, addr);
    uint16_t port = sockets::networkToHost16(addr.sin_port);
    snprintf(buf, size, "%s:%u", host, port);   
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

int sockets::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = sizeof optval;

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else 
    {
        return optval;
    }
}

struct sockaddr_in sockets::getLocalAddr(int sockfd)
{
    struct sockaddr_in localAddr;
    bzero(&localAddr, sizeof localAddr);
    socklen_t addrlen = sizeof localAddr;
    if (::getsockname(sockfd, sockaddr_cast(&localAddr), &addrlen) < 0)
    {
        LOG_SYSERR << "sockets::getLocalAddr";
    }
    return localAddr;
}

struct sockaddr_in sockets::getPeerAddr(int sockfd)
{
    struct sockaddr_in peerAddr;
    bzero(&peerAddr, sizeof peerAddr);
    socklen_t addrlen = sizeof peerAddr;
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
    struct sockaddr_in localAddr = getLocalAddr(sockfd);
    struct sockaddr_in peerAddr = getPeerAddr(sockfd);
    return localAddr.sin_addr.s_addr == peerAddr.sin_addr.s_addr  &&
            localAddr.sin_port == peerAddr.sin_port;
}
