/*
 * @Author: py.wang 
 * @Date: 2019-05-25 09:07:26 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-27 08:25:06
 */

#include <muduo/net/InetAddress.h>
#include <muduo/net/Endian.h>
#include <muduo/net/SocketsOps.h>

#include <strings.h>
#include <netinet/in.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny =  INADDR_ANY;
#pragma GCC diagnostic error "-Wold-style-cast"

using namespace muduo;
using namespace muduo::net;

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in));

InetAddress::InetAddress(uint16_t port)
{
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = sockets::hostToNetwork32(kInaddrAny);
    addr_.sin_port = sockets::hostToNetwork16(port);
}

InetAddress::InetAddress(const StringPiece &ip, uint16_t port)
{
    bzero(&addr_, sizeof addr_);
    sockets::fromIpPort(ip.data(), port, &addr_);
}

string InetAddress::toIpPort() const 
{
    char buf[32];
    sockets::toIpPort(buf, sizeof buf, addr_);
    return buf;
}

string InetAddress::toIp() const 
{
    char buf[32];
    sockets::toIp(buf, sizeof buf, addr_);
    return buf;
}

uint16_t InetAddress::toPort() const 
{
    return sockets::networkToHost16(portNetEndian());
}


