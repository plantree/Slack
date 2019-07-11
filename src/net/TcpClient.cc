/*
 * @Author: py.wang 
 * @Date: 2019-06-03 07:48:47 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-08 09:40:53
 */
#include "src/net/TcpClient.h"
#include "src/net/Connector.h"
#include "src/net/SocketsOps.h"
#include "src/net/EventLoop.h"
#include "src/log/Logging.h"

#include <functional>

#include <stdio.h>

using namespace slack;
using namespace slack::net;

using namespace std::placeholders;

namespace slack
{
    
namespace net
{

namespace detail
{
    // 移除连接
    void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn)
    {
        loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
    void removeConnector(const ConnectorPtr &connector)
    {
        // do nothing temporaily
    }
    
}   // namespace detail

}   // namespace net

}   // namespace muduo

TcpClient::TcpClient(EventLoop *loop,
                    const InetAddress &serverAddr,
                    const string &nameArg)
    :loop_(CHECK_NOTNULL(loop)),
    connector_(new Connector(loop, serverAddr)),
    name_(nameArg),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),
    retry_(false),
    connect_(true),
    nextConnId_(1)
{
    connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, _1));
    LOG_INFO << "TcpClient::TcpClient[" << name_
            << "] - connector " << get_pointer(connector_);
}

TcpClient::~TcpClient()
{
    // 清除connection
    LOG_INFO << "TcpClient::~TcpClient[" << name_
            << "] - connector " << get_pointer(connector_);
    // 避免connection过早析构?
    TcpConnectionPtr conn;
    bool unique = false;
    {
        MutexLockGuard lock(mutex_);
        unique = connection_.unique();
        conn = connection_;
    }
    if (conn)
    {
        assert(loop_ == conn->getLoop());
        // FIXME: not 100% safe, if we are in different thread
        CloseCallback cb = std::bind(&detail::removeConnection, loop_, _1);
        loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
        if (unique)
        {
            conn->forceClose();
        }
    }
    else 
    {
        // 这时说明connector尚未连接
        connector_->stop();
        // TODO??
        loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
    }
}

void TcpClient::connect()
{
    LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to " 
        << connector_->servAddress().toIpPort();
    connect_ = true;
    connector_->start();
}

// 用于连接已经建立的情况，关闭连接
void TcpClient::disconnect()
{
    connect_ = false;

    {
        MutexLockGuard lock(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

// 停止connector_
void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    InetAddress localAddr(sockets::getLocalAddr(sockfd));

    // FIXME poll with zero timeout to double confirm the new connection
    // use make_shared if necessary
    TcpConnectionPtr conn(new TcpConnection(loop_,
                                        connName,
                                        sockfd,
                                        localAddr,
                                        peerAddr));
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, _1));
    {
        MutexLockGuard lock(mutex_);
        connection_ = conn; // 保存TcpConnection
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        MutexLockGuard lock(mutex_);
        assert(connection_ == conn);
        // 连接重置
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if (retry_ && connect_)
    {
        LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to "
            << connector_->servAddress().toIpPort();
        connector_->restart();
    }
}


