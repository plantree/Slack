/*
 * @Author: py.wang 
 * @Date: 2019-05-25 08:55:26 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-27 08:24:34
 */

#ifndef NET_INETADDRESS_H
#define NET_INETADDRESS_H

#include "src/base/copyable.h"
#include "src/base/StringPiece.h"

#include <netinet/in.h>

namespace slack
{

namespace net
{

namespace sockets
{
    const struct sockaddr *sockaddr_cast(const struct sockaddr_in6 *addr);
}

// wrapper of sockaddr_in
// POD interface class (可以与C兼容)
class InetAddress : public copyable
{
public:
    // constructs an endpoint with given port number,
    // mostly used in TcpServer listening
    // 不指定ip则为INADDR_ANY
    explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false,
                        bool ipv6 = false);

    // constructs an endpoint with given ip and port
    InetAddress(const StringArg ip, uint16_t port, bool ipv6 = false);

    // constructs an endopoint with given struct @c sockaddr_in
    // mostly used when accepting new connections
    explicit InetAddress(const struct sockaddr_in &addr)
        : addr_(addr)
    {
    }

    explicit InetAddress(const struct sockaddr_in6 &addr)
        : addr6_(addr)
    {
    }

    sa_family_t family() const 
    {
        return addr_.sin_family;
    }
    string toIp() const;
    string toIpPort() const;
    uint16_t toPort() const;

    // __attribute__((deprecated))表示函数是过时的
    // 编译器会发出警告
    string toHostPort() const __attribute__ ((deprecated))
    {
        return toIpPort();
    }

    // default copy/assignment are okay

    const struct sockaddr *getSockAddr() const 
    {
        return sockets::sockaddr_cast(&addr6_);
    }
    void setSockAddrInet6(const struct sockaddr_in6 &addr6)
    {
        addr6_ = addr6;
    }

    uint32_t ipNetEndian() const;
    uint16_t portNetEndian() const
    {
        return addr_.sin_port;
    }

    // resolve hostname to IP address, not changing port of sin_family
    // return true on success
    // thread safe
    static bool resolve(StringArg hostName, InetAddress *result);

    // set IPv6 ScopeID
    void setScopeId(uint32_t scope_id);

private:
    union 
    {
        struct sockaddr_in addr_;
        struct sockaddr_in6 addr6_;
    };
};

}   // namespace net

}   // namespace slack

#endif // NET_INETADDRESS_H