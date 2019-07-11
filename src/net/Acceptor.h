/*
 * @Author: py.wang 
 * @Date: 2019-05-27 08:44:32 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-28 08:19:48
 */
#ifndef NET_ACCEPTOR
#define NET_ACCEPTRO

#include "src/net/Channel.h"
#include "src/net/Socket.h"

#include <functional>

namespace slack
{

namespace net
{

class EventLoop;
class InetAddress;

// Acceptor of incoming TCP connection
class Acceptor : noncopyable
{
public:
    // 连接回调，（sockfd， address）
    using NewConnectionCallback = std::function<void(int sockfd, 
                                                    const InetAddress &)>;
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reusePort);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = cb;
    }

    bool listenning() const 
    {
        return listenning_;
    }
    void listen();

private:
    void handleRead();

    EventLoop *loop_;

    Socket acceptSocket_;   // connetion socket
    Channel acceptChannel_; 
    
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
    int idleFd_;    // 处理已用文件描述符过多的情况
};

}   // namesapce net

}   // namespace slack

#endif  // NET_ACCEPTOR