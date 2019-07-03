/*
 * @Author: py.wang 
 * @Date: 2019-05-18 08:58:45 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-19 09:34:17
 */

#include "src/net/Poller.h"
#include "src/net/poller/EpollPoller.h"

#include <stdio.h>

using namespace slack::net;

// 作为一个中间层，生成合适的Poller
Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    return new EpollPoller(loop);
}