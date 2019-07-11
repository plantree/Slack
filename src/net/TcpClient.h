/*
 * @Author: py.wang 
 * @Date: 2019-06-03 07:39:08 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-08 09:27:49
 */
#ifndef NET_TCPCLIENT_H
#define NET_TCPCLIENT_H

#include "src/base/noncopyable.h"
#include "src/base/Mutex.h"
#include "src/net/TcpConnection.h"

#include <memory>

namespace slack
{

namespace net
{

class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class TcpClient : noncopyable
{
public:
    TcpClient(EventLoop *loop,  
                const InetAddress &servAddr,
                const string &nameArg);
    ~TcpClient();

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection() const 
    {
        MutexLockGuard lock(mutex_);
        return connection_;
    }

    EventLoop *getLoop() const 
    {
        return loop_;
    }
    bool retry() const
    {
        return retry_;
    }
    void enableRetry()
    {
        retry_ = true;
    }

    // set connection callback
    // not thread safe
    void setConnectionCallback(ConnectionCallback cb)
    {
        connectionCallback_ = std::move(cb);
    }

    // set meesage callback
    // not thread safe
    void setMessageCallback(MessageCallback cb)
    {
        messageCallback_ = std::move(cb);
    }

    // set write complete callback
    // not thread safe
    void setWriteCompleteCallback(WriteCompleteCallback cb)
    {
        writeCompleteCallback_ = std::move(cb);
    }
    
private:
    // not thread safe, but in loop
    void newConnection(int sockfd);
    // not thread safe, but in loop
    void removeConnection(const TcpConnectionPtr &conn);

    EventLoop *loop_;
    ConnectorPtr connector_;    // 主动发起连接
    const string name_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    
    bool retry_;    // 连接断开后是否重连
    bool connect_;
    // always in loop thread
    int nextConnId_;
    mutable MutexLock mutex_;
    TcpConnectionPtr connection_;   // Connector连接成功得到一个TcpConnectionPtr
};

}   // namespace net

}   // namespace muduo

#endif   // MUDUO_NET_TCPCLIENT_H