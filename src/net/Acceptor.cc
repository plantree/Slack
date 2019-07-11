/*
 * @Author: py.wang 
 * @Date: 2019-05-27 08:50:47 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-27 09:12:37
 */

#include "src/net/Acceptor.h"
#include "src/net/EventLoop.h"
#include "src/net/InetAddress.h"
#include "src/net/SocketsOps.h"
#include "src/log/Logging.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

using namespace slack;
using namespace slack::net;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reusePort)
    : loop_(loop),
    acceptSocket_(sockets::createNonBlockingOrDie(listenAddr.family())),
    acceptChannel_(loop, acceptSocket_.fd()),
    listenning_(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(idleFd_ >= 0);
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reusePort);
    acceptSocket_.bindAddress(listenAddr);
    // 设置读回调
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    // 移除Channel
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listenning_ = true;
    // listen
    acceptSocket_.listen();
    // 读事件注册到底层Poller
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    InetAddress peerAddr;
    // FIXME loop until no more
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        string hostport = peerAddr.toIpPort();
        LOG_TRACE << "Accepts of " << hostport;
        // 存在新连接回调
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr);
        }
        else 
        {
            // 否则关闭连接
            sockets::close(connfd);
        }
    }
    else 
    {
        LOG_SYSERR << "in Acceptor::handleRead";
        // 已用的文件描述符号超过系统限制
        // 接收连接然后关闭，不服务
        if (errno == EMFILE)
        {
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}