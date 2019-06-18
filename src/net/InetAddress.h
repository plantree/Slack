/*
 * @Author: py.wang 
 * @Date: 2019-05-25 08:55:26 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-27 08:24:34
 */

#ifndef MUDUO_NET_INETADDRESS_H
#define MUDUO_NET_INETADDRESS_H

#include <muduo/base/copyable.h>
#include <muduo/base/StringPiece.h>

#include <netinet/in.h>

namespace muduo
{

namespace net
{

// wrapper of sockaddr_in
// POD interface class (可以与C兼容)
class InetAddress : public copyable
{
public:
    // constructs an endpoint with given port number,
    // mostly used in TcpServer listening
    // 不指定ip则为INADDR_ANY
    explicit InetAddress(uint16_t port);

    // constructs an endpoint with given ip and port
    InetAddress(const StringPiece &ip, uint16_t port);

    // constructs an endopoint with given struct @c sockaddr_in
    // mostly used when accepting new connections
    InetAddress(const struct sockaddr_in &addr)
        : addr_(addr)
    {
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

    const struct sockaddr_in &getSockAddrInet() const 
    {
        return addr_;
    }
    void setSockAddrInet(const struct sockaddr_in &addr)
    {
        addr_ = addr;
    }

    uint32_t ipNetEndian() const 
    {
        return addr_.sin_addr.s_addr;
    }
    uint16_t portNetEndian() const 
    {
        return addr_.sin_port;
    }
private:
    struct sockaddr_in addr_;
};

}   // namespace net

}   // namespace muduo

#endif // MUDUO_NET_INETADDRESS_H