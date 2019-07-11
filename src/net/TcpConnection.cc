/*
 * @Author: py.wang 
 * @Date: 2019-05-28 07:48:33 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-10 09:42:08
 */

#include "src/net/TcpConnection.h"
#include "src/net/Channel.h"
#include "src/net/EventLoop.h"
#include "src/net/Socket.h"
#include "src/net/SocketsOps.h"
#include "src/log/Logging.h"
#include "src/base/WeakCallback.h"

#include <functional>

#include <errno.h>
#include <stdio.h>

using namespace slack;
using namespace slack::net;

using namespace std::placeholders;

// 默认连接回调
void slack::net::defaultConnectionCallback(const TcpConnectionPtr &conn)
{
    LOG_TRACE << conn->localAddress().toIpPort() << "->"
            << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
}

// 默认读事件回调
void slack::net::defaultMessageCallback(const TcpConnectionPtr &conn, 
                                        Buffer *buf, 
                                        Timestamp)
{
    buf->retrieveAll();
}


// ctor
TcpConnection::TcpConnection(EventLoop *loop,
                            const string &nameArg,
                            int sockfd,
                            const InetAddress &localAddr,
                            const InetAddress &peerAddr)
    : loop_(CHECK_NOTNULL(loop)),
    name_(nameArg),
    state_(kConnecting),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64 * 1024 * 1024)
{
    // Channel可读事件到来，回调handleRead
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
    // 读通道可写事件到达
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    // 连接关闭，回调handleClose
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    // 发生错误，回调handleError
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    
    LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this 
            << " fd = " << sockfd;
    // 设置为保持连接
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
            << " fd = " << channel_->fd()
            << " state = " << stateToString();
    assert(state_ == kDisconnected);
}

bool TcpConnection::getTcpInfo(struct tcp_info *tcpi) const 
{
    return socket_->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const 
{
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof buf);
    return buf;
}

// 线程安全，可以跨线程调用
void TcpConnection::send(const void *data, int len)
{
    send(StringPiece(static_cast<const char *>(data), len));
}

// 线程安全，可以跨线程调用
void TcpConnection::send(const StringPiece &message)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message);
        }
        else 
        {
            void (TcpConnection::*fp)(const StringPiece &message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(fp, 
                                       this, 
                                       message.as_string()));
        }
    }
}

// 线程安全，可以跨线程调用
void TcpConnection::send(Buffer *buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else 
        {
            void (TcpConnection::*fp)(const StringPiece &message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                std::bind(fp,
                        this,
                        buf->retrieveAllAsString()));
        }
    }
}

void TcpConnection::sendInLoop(const StringPiece &message)
{
    // 委托
    sendInLoop(message.data(), message.size());
}

// 写
// 写之前没有关注写事件，写后关注
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == kDisconnected)
    {
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    // if no thing in output queue, try writng directly
    // channel_没有监听写事件，写缓冲区空则直写
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = sockets::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            // 写完了回调writeCompleteCallback
            if (remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else    // nwrote < 0 
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                // 管道错误和连接重置
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    assert(remaining <= len);
    // 没有错误，并且还有没写完的数据（说明内核发送缓冲区满，添加到outputBuffer_）
    if (!faultError && remaining > 0)
    {
        //LOG_TRACE << "I am going to write more data";
        size_t oldLen = outputBuffer_.readableBytes();
        // 如果超过highWaterMark_,回调
        if (oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();  // 关注POLLOUT
        }
    }
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        // FIXME: shard_from_this()?
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting())
    {
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceClose()
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        setState(kDisconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        setState(kDisconnecting);
        loop_->runAfter(
            seconds,
            makeWeakCallback(shared_from_this(),
                                &TcpConnection::forceClose));   // not forceCloseInLoop to avoid
                                                                // race condition
    }
}

void TcpConnection::forceCloseInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        // as if we received 0 byte in handleRead()
        handleClose();
    }
}

const char *TcpConnection::stateToString() const 
{
    switch (state_)
    {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
    loop_->assertInLoopThread();
    if (!isReading() || !channel_->isReading())
    {
        channel_->enableReading();
        reading_ = true;
    }
}

void TcpConnection::stopRead()
{
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
    loop_->assertInLoopThread();
    if (reading_ || channel_->isReading())
    {
        channel_->disableReading();
        reading_ = false;
    }
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    // 增加智能指针引用技术，保证比Channel的存活时间久
    // 已经有一个share_ptr被构建，放在TcpServer的map里
    // enable_shared必须之前已经有现成的shared_ptr
    channel_->tie(shared_from_this());
    channel_->enableReading();  // 对应Channel加入到Poller监听

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
    // 从Poller监听中移除
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else 
    {
        errno = savedErrno;
        LOG_ERROR << "TcoConnection::handleRead";
        handleError();
    }
}

// 内核发送缓冲区有空间，回调该函数
void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (channel_->isWriting())
    {
        ssize_t n = sockets::write(channel_->fd(),  
                                    outputBuffer_.peek(),
                                    outputBuffer_.readableBytes());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            // 发送区缓冲区清空
            if (outputBuffer_.readableBytes() == 0)
            {
                // 及时关闭写事件
                channel_->disbaleWriting(); // 停止关注POLLOUT，避免busy loop
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();   // 关闭连接
                }
            }
        }
        else 
        {
            LOG_SYSERR << "TcpConnection::handleWrite";
        }
    }
    else 
    {
        LOG_TRACE << "Connection fd = " << channel_->fd()
                << " is down, no more writing";
    }
}


void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "fd = " << channel_->fd() << " state = " << state_;
    assert(state_ == kConnected || state_ == kDisconnecting);
    // we don't close fd, leave it to dtor
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);
    closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_
        << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}