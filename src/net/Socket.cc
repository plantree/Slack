/*
 * @Author: py.wang 
 * @Date: 2019-05-26 10:49:15 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-26 11:04:17
 */
#include "src/net/Socket.h"
#include "src/net/InetAddress.h"
#include "src/net/SocketsOps.h"
#include "src/log/Logging.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>    // snprintf

using namespace slack;
using namespace slack::net;

Socket::~Socket()
{
    sockets::close(sockfd_);
}

bool Socket::getTcpInfo(struct tcp_info *tcpi) const 
{
    socklen_t len = sizeof(*tcpi);
    memZero(tcpi, len);
    return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::getTcpInfoString(char *buf, int len) const 
{   
    struct tcp_info tcpi;
    bool ok = getTcpInfo(&tcpi);
    if (ok)
    {
        snprintf(buf, len, "unrecovered=%u "
                "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
                "lost=%u retrans=%u cwnd=%u total_retrans=%u"
                "sshthresh=%u cwnd=%u total_retrans=%u",
                tcpi.tcpi_retransmits,  // number of unrecovered [RTO] timeouts
                tcpi.tcpi_rto,          // retransmit timeout in usec
                tcpi.tcpi_ato,          // predicted tick of soft clock in usec
                tcpi.tcpi_snd_mss,      
                tcpi.tcpi_rcv_mss,
                tcpi.tcpi_lost,         // lost packets
                tcpi.tcpi_retrans,      // retransmitted packets out
                tcpi.tcpi_rtt,          // smoothed round trip time in usec
                tcpi.tcpi_rttvar,       // medium deviaion
                tcpi.tcpi_snd_ssthresh,
                tcpi.tcpi_snd_cwnd,
                tcpi.tcpi_total_retrans); // total retransmits for entire connection 
    }
    return ok;
}


void Socket::bindAddress(const InetAddress &addr)
{
    sockets::bindOrDie(sockfd_, addr.getSockAddr());
}

void Socket::listen()
{
    sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress *peerAddr)
{
    struct sockaddr_in6 addr;
    memZero(&addr, sizeof addr);
    int connfd = sockets::accept(sockfd_, &addr);
    if (connfd >= 0)
    {
        peerAddr->setSockAddrInet6(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    sockets::shutDownWrite(sockfd_);
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
                &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0)
    {
        LOG_SYSERR << "setTcpNoDelay";
    }   
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
                &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0)
    {
        LOG_SYSERR << "setReuseAddr";
    }   
}

void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
                &optval, static_cast<socklen_t>(sizeof optval));
    if (ret && on)
    {
        LOG_SYSERR << "setReusePort";
    }
#else 
    if (on)
    {
        LOG_ERROR << "setReusePort is not supported";
    }
#endif
}

// 检测对端主机是否崩溃或不可达
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
                &optval, sizeof optval);
    if (ret < 0)
    {
        LOG_SYSERR << "setKeepAlive";
    }   
}