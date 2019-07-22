/*
 * @Author: py.wang 
 * @Date: 2019-05-27 09:25:10 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-22 07:48:13
 */

#ifndef NET_TCPCONNECTION_H
#define NET_TCPCONNECTION_H

#include "src/base/Mutex.h"
#include "src/base/StringPiece.h"
#include "src/base/Types.h"
#include "src/base/noncopyable.h"
#include "src/net/Callbacks.h"
#include "src/net/InetAddress.h"
#include "src/net/Buffer.h"
#include "src/net/any.h"

#include <functional>
#include <memory>

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace slack
{

namespace net
{

class Channel;
class EventLoop;
class Socket;

// Tcp connection, for both client and server usage
// This is an interface class, so don't expose too much details
class TcpConnection : noncopyable,
                    public std::enable_shared_from_this<TcpConnection> // 在类的内部获取shared_ptr
{
public:
    // construct a TcpConnection with a connected sockfd
    TcpConnection(EventLoop *loop,
                    const string &name,
                    int sockfd,
                    const InetAddress &localAddr,
                    const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const 
    {
        return loop_;
    }

    const string &name() const 
    {
        return name_;
    }

    const InetAddress &localAddress()
    {
        return localAddr_;
    }

    const InetAddress &peerAddress()
    {
        return peerAddr_;
    }

    bool connected() const 
    {
        return state_ == kConnected;
    }

    bool disconnetected() const 
    {
        return state_ == kDisconnected;
    }

    // return true if success
    bool getTcpInfo(struct tcp_info *) const;
    string getTcpInfoString() const;

    void send(const void *message, int len);
    void send(const StringPiece &message);
    void send(Buffer *message);

    void shutdown();    // not thread safe, no simultaneous calling
    
    void forceClose();
    void forceCloseWithDelay(double seconds);
    
    void setTcpNoDelay(bool on);

    void startRead();
    void stopRead();
    bool isReading() const 
    {
        return reading_;
    }

    void setContext(const any &context)
    {
        context_ = context;
    }

    const any &getContext() const 
    {
        return context_;
    }

    any *getMutableContext()
    {
        return &context_;
    }

    // 回调函数：连接、读事件、写完成事件、高水位事件
    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback &cb)
    {
        messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }

    void setHightWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    // advance interface
    Buffer *inputBuffer()
    {
        return &inputBuffer_;
    }
    Buffer *outputBuffer()
    {
        return &outputBuffer_;
    }

    // Internal use only
    // 关闭时回调
    void setCloseCallback(const CloseCallback &cb)
    {
        closeCallback_ = cb;
    }

    // called when TcpServer accepts a new connection
    void connectEstablished();  // should be called only once
    // called when TcpServer has removed me from its map
    void connectDestroyed();
    
private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting};
    
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const StringPiece &message);
    void sendInLoop(const void *message, size_t len);
    
    void shutdownInLoop();
    void forceCloseInLoop();
    void setState(StateE s)
    {
        state_ = s;
    }
    const char *stateToString() const;
    void startReadInLoop();
    void stopReadInLoop();

    EventLoop *loop_;
    const string name_;
    StateE state_;  // FIXME: use atomic variable
    bool reading_;

    // we don't expose those classes to client
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    
    InetAddress localAddr_;
    InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    // 数据发送完毕回调函数，就是所有的用户数据都拷贝到内核缓冲区，
    // outputBuffer_被清空也会回调该函数，可理解为低水位回调函数
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;   // 高水位标回调函数
    CloseCallback closeCallback_;

    size_t highWaterMark_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;

    any context_;
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

}   // namespace net

}   // namespace muduo

#endif  // MUDUO_NET_TCPCONNECTION_H
