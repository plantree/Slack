/*
 * @Author: py.wang 
 * @Date: 2019-05-25 09:10:24 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-25 09:18:39
 */

#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include "src/base/noncopyable.h"

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace slack
{
// TCP networking
namespace net
{
// 前向声明
class InetAddress;

// wrapper of socket file descriptor
// it closed the sockfd when destructs
// thread safe, all operations are delegated to OS

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {
    }

    ~Socket();

    int fd() const 
    {
        return sockfd_;
    }

    // return true if success
    bool getTcpInfo(struct tcp_info *) const;
    bool getTcpInfoString(char *buf, int len) const;
    
    // abort if address in use
    void bindAddress(const InetAddress &localAddr);
    // abort if address in use
    void listen();

    // on success, returns a non-negative integer that is
    // a descriptor for the accepted socket, which has been
    // set to non-blocking and close-on-exec. *peeraddr is assigned
    // On error, -1 is returned and *peeraddr is untouched
    int accept(InetAddress *peerAddr);

    void shutdownWrite();

    // Enable/disable TCP_NODELY(Nagel算法)
    void setTcpNoDelay(bool on);

    // Enable/disbale SO_REUSEADDR
    void setReuseAddr(bool on);

    // Enable/disable SO_REUSEPORT
    void setReusePort(bool on);

    // Enable/disbale SO_KEEPALIVE
    // 指定期探测连接是否存在，如果应用层有心跳，就不必设置
    void setKeepAlive(bool on);
    
private:
    const int sockfd_;
};

}   // namespace net

}   // namespace slack

#endif  // NET_SOCKET_H