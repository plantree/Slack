/*
 * @Author: py.wang 
 * @Date: 2019-05-28 08:12:16 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-07 10:59:55
 */
#include "src/net/TcpServer.h"
#include "src/net/Acceptor.h"
#include "src/net/EventLoop.h"
#include "src/net/EventLoopThreadPool.h"
#include "src/net/SocketsOps.h"
#include "src/log/Logging.h"

#include <functional>

#include <stdio.h>  // snprintf

using namespace slack;
using namespace slack::net;

using namespace std::placeholders;

TcpServer::TcpServer(EventLoop *loop,
                    const InetAddress &listenAddr,
                    const string &nameArg,
                    Option option)
    : loop_(CHECK_NOTNULL(loop)),
    ipPort_(listenAddr.toIpPort()),
    name_(nameArg),
    acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
    threadPool_(new EventLoopThreadPool(loop, name_)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),
    nextConnId_(1)
{
    // Acceptor::handleRead回调TcpServer::newConnection
    // _1对应socket文件描述符，_2对应peerAddress
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

    for (auto &it : connections_)
    {
        // 多一次引用
        TcpConnectionPtr conn = it.second;
        // 释放当前控制资源，减少一次引用计数
        it.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
        conn.reset();  
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    assert(numThreads >= 0);
    threadPool_->setThreadNum(numThreads);
}

// 可以多次调用，跨线程也可以
void TcpServer::start()
{
    // 只能开始一次
    if (started_.getAndSet(1) == 0)
    {
        // 启动线程池
        threadPool_->start(threadInitCallback_);

        // 开始监听
        assert(!acceptor_->listenning());
        loop_->runInLoop(std::bind(&Acceptor::listen, get_pointer(acceptor_)));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    loop_->assertInLoopThread();
    // 轮询选择下一个
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    LOG_INFO << "TcpServer::newConnection [" << name_
            << "] - new connection [" << connName
            << "] from " << peerAddr.toIpPort();
    InetAddress localAddr(sockets::getLocalAddr(sockfd));

    // 创建连接
    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));
    connections_[connName] = conn;
    // 设置回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));

    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
            << "] - connection " << conn->name();

    size_t n = connections_.erase(conn->name()); (void)n;
    assert(n == 1);

    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
