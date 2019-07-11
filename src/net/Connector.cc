/*
 * @Author: py.wang 
 * @Date: 2019-06-02 10:21:19 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-07 13:51:46
 */
#include "src/net/Connector.h"
#include "src/net/Channel.h"
#include "src/net/EventLoop.h"
#include "src/net/Channel.h"
#include "src/net/SocketsOps.h"
#include "src/log/Logging.h"

#include <functional>

#include <errno.h>

using namespace slack;
using namespace slack::net;

// 静态成员定义
const int Connector::kMaxRetryDelayMs;
const int Connector::kInitRetryDelayMs;

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop),
    serverAddr_(serverAddr),
    connect_(false),
    state_(kDisconnected),
    retryDelayMs_(kInitRetryDelayMs)
{
    LOG_DEBUG << "Connector ctor[" << this << "]";
}

Connector::~Connector()
{
    LOG_DEBUG << "Connector dtro[" << this << "]";
    // 生存期长于channel
    assert(!channel_);
}

// 可以跨线程调用
void Connector::start()
{
    // 允许连接
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this)); // FIXME unsafe
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_)
    {
        connect();
    }
    else 
    {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::stop()
{
    // 停止连接
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));    // FIXME unsafe
}

void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    // 如果正在连接
    if (state_ == kConnecting)
    {
        setState(kDisconnected);    // 设置为断开连接
        int sockfd = removeAndResetChannel();   // Poller移除关注，channel置空
        retry(sockfd);  // 这里不是重连，调用socketsL::close(sockfd)
    }
}

void Connector::connect()
{
    // connect
    int sockfd = sockets::createNonBlockingOrDie(serverAddr_.family()); // 创建非阻塞套接字
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());
    // handle error
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS:   // 表示正在连接
        case EINTR:
        case EISCONN:   // 连接成功
            connecting(sockfd);
            break;
        
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd);
            break;
        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            break;
        
        default:
            LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            break;
    }
}

// 不能跨线程调用
void Connector::restart()
{
    // 状态重置
    loop_->assertInLoopThread();
    setState(kDisconnected);
    // 超时重置
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    // 重新开始
    startInLoop();
}

void Connector::connecting(int sockfd)
{
    // 正在连接
    setState(kConnecting);
    assert(!channel_);

    // Channel关联sockfd
    channel_.reset(new Channel(loop_, sockfd));
    // 设置可写回调
    channel_->setWriteCallback(std::bind(&Connector::handleWrite, this));
    // 设置错误回调
    channel_->setErrorCallback(std::bind(&Connector::handleError, this));

    channel_->enableWriting();
}

int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();     // 取消关注
    int sockfd = channel_->fd();
    // cannot reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
    return sockfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}

void Connector::handleWrite()
{
    // 写事件到来说明连接即将建立
    LOG_TRACE << "Connector::handleWrite " << state_;

    if (state_ == kConnecting)
    {
        // 连接一旦建立，就可以取消channel关注了
        int sockfd = removeAndResetChannel();
        // socket可写并不意味着连接一定建立成功
        int err = sockets::getSocketError(sockfd);
        if (err)
        {
            LOG_WARN << "Connector::handleWrite - SO_ERROR = "
                << err << " " << strerror_tl(err);
            retry(sockfd);  // 重连
        }
        else if (sockets::isSelfConnect(sockfd)) // 自连接
        {
            LOG_WARN << "Connector::handleWrite - Self connect";
            retry(sockfd);
        }
        else 
        {
            // 连接成功
            setState(kConnected);
            if (connect_)
            {
                newConnectionCallback_(sockfd);
            }
            else 
            {
                sockets::close(sockfd);
            }
        }
    }
    else 
    {
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError()
{
    LOG_ERROR << "Connector::handleError";
    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
        retry(sockfd);
    }
}

// 采用back-off策略重连
void Connector::retry(int sockfd)
{
    // 状态重置
    sockets::close(sockfd);
    setState(kDisconnected);
    if (connect_)
    {
        LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort() 
                << " in " << retryDelayMs_ << " milliseconds. ";
        // 注册一个定时操作，重连
        loop_->runAfter(retryDelayMs_/1000.0, std::bind(&Connector::startInLoop, shared_from_this()));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }
    else 
    {
        LOG_DEBUG << "do not connect";
    }
}

