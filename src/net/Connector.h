/*
 * @Author: py.wang 
 * @Date: 2019-06-02 10:12:09 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-08 09:32:42
 */
#ifndef NET_CONNECTOR
#define NET_CONNECTOR

#include "src/net/InetAddress.h"
#include "src/base/noncopyable.h"

#include <functional>
#include <memory>

namespace slack
{

namespace net
{

class Channel;
class EventLoop;

// enable_shared_from_this
class Connector : noncopyable, public std::enable_shared_from_this<Connector>
{
public:
    using NewConnectionCallback = std::function<void (int sockfd)>;

    Connector(EventLoop *loop, const InetAddress &servAddr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = cb;
    }

    void start();   // can be called in any thread
    void restart(); // must be called in loop thread
    void stop();    // can be called in any thread

    const InetAddress &servAddress() const 
    {
        return serverAddr_;
    }

private:
    enum State {kDisconnected, kConnecting, kConnected};
    static const int kMaxRetryDelayMs = 30 * 1000;  // 30s
    static const int kInitRetryDelayMs = 500;   // 500ms

    void setState(State s)
    {
        state_ = s;
    }
    void startInLoop();
    void stopInLoop();

    void connect();
    void connecting(int sockfd);

    void handleWrite();
    void handleError();

    void retry(int sockfd);
    int removeAndResetChannel();
    void resetChannel();

    EventLoop *loop_;
    InetAddress serverAddr_;
    bool connect_;  // atomic
    State state_;
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
    int retryDelayMs_;  // 重连延迟时间

};

}   // namespace net

}   // namespace slack


#endif  // NET_CONNECTOR