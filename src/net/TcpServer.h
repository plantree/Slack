/*
 * @Author: py.wang 
 * @Date: 2019-05-28 08:03:43 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-07 10:48:04
 */
#ifndef NET_TCPSERVER_H
#define NET_TCPSERVER_H

#include "src/base/Atomic.h"
#include "src/base/Types.h"
#include "src/net/TcpConnection.h"

#include <map>
#include <memory>

namespace slack
{

namespace net
{

// 前向声明
class Acceptor;
class EventLoop;
class EventLoopThreadPool;

// Tcp Server, supports single-threaded and thread-pool models
// This is an interface class, so don't expose too much details
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void (EventLoop*)>;
    enum Option
    {
        kNoReusePort,
        kReusePort
    };

    TcpServer(EventLoop *loop,
                const InetAddress &listenAddr,
                const string &nameArg,
                Option option = kNoReusePort);
    ~TcpServer();

    const string &ipPort() const 
    {
        return ipPort_;
    }

    const string &name() const 
    {
        return name_;
    }

    EventLoop *getLoop() const 
    {
        return loop_;
    }

    // set the number of threads for handling input
    // always accepts new connection in loop's thread
    void setThreadNum(int numThreads);
    void setThreadInitCallback(const ThreadInitCallback &cb)
    {
        threadInitCallback_ = cb;
    }

    // valid after calling start()
    std::shared_ptr<EventLoopThreadPool> threadPool()
    {
        return threadPool_;
    }

    // starts the server if it's not listenning
    // it's harmless to call if multiple times
    void start();

    // set connection callback, not thread safe
    void setConnectionCallbck(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }

    // set message callback, not thread safe
    void setMessageCallback(const MessageCallback &cb)
    {
        messageCallback_ = cb;
    }

    // not thread safe
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }
private:
    // Not thread safe, but in loop
    void newConnection(int sockfd, const InetAddress &peerAddr);
    // thread safe
    void removeConnection(const TcpConnectionPtr &conn);
    // not thread safe, but in loop
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    typedef std::map<string, TcpConnectionPtr> ConnectionMap;

    EventLoop *loop_;   // the acceptor loop
    const string ipPort_;
    const string name_;

    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;
    
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;   // 数据发送完毕回调函数
    ThreadInitCallback threadInitCallback_;
    
    AtomicInt32 started_;
    // always in loop thread
    int nextConnId_;
    ConnectionMap connections_;

};

}   // namespace net

}   // namespace muduo

#endif  // MUDUO_NET_TCPSERVER_H