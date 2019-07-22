/*
 * @Author: py.wang 
 * @Date: 2019-07-21 08:59:19 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-21 09:06:06
 */
#ifndef NET_HTTP_HTTPSERVER_H
#define NET_HTTP_HTTPSERVER_H

#include "src/net/TcpServer.h"

namespace slack
{

namespace net
{

class HttpRequest;
class HttpResponse;

class HttpServer : noncopyable 
{
public:
    using HttpCallback = std::function<void (const HttpRequest &,
                                            HttpResponse *)>;
    HttpServer(EventLoop *loop,
                const InetAddress &listenAddr,
                const string &name,
                TcpServer::Option option = TcpServer::kNoReusePort);
    
    EventLoop *getLoop() const 
    {
        return server_.getLoop();
    }

    // not thread safe, callback be registerd before calling start()
    void setHttpCallback(const HttpCallback &cb)
    {
        httpCallback_ = cb;
    }

    void setThreadNum(int numThreads)
    {
        server_.setThreadNum(numThreads);
    }

    void start();

private:
    void onConnection(const TcpConnectionPtr &conn);
    void onMessage(const TcpConnectionPtr &conn,
                    Buffer *buf,
                    Timestamp receiveTime);
    void onRequest(const TcpConnectionPtr &, const HttpRequest &);

    TcpServer server_;
    HttpCallback httpCallback_;

};

}   // namespace net

}   // namespace slack


#endif  // NET_HTTP_HTTPSERVER_H