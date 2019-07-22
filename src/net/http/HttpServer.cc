/*
 * @Author: py.wang 
 * @Date: 2019-07-21 09:06:22 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-22 08:42:32
 */
#include "src/net/http/HttpServer.h"
#include "src/net/http/HttpContext.h"
#include "src/net/http/HttpRequest.h"
#include "src/net/http/HttpResponse.h"
#include "src/log/Logging.h"

using namespace slack;
using namespace slack::net;

namespace slack
{

namespace net
{

namespace detail
{

void defaultHttpCallback(const HttpRequest &, HttpResponse *response)
{
    response->setStatusCode(HttpResponse::k404NotFound);
    response->setStatusMessage("Not Found");
    response->setCloseConnection(true);
}

}   // namespace detail

}   // namespace net

}   // namespace slack

HttpServer::HttpServer(EventLoop *loop,
                        const InetAddress &listenAddr,
                        const string &name,
                        TcpServer::Option option)
    : server_(loop, listenAddr, name, option),
    httpCallback_(detail::defaultHttpCallback)
{
    server_.setConnectionCallbck(std::bind(&HttpServer::onConnection, this, _1));
    server_.setMessageCallback(std::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

void HttpServer::start()
{
    LOG_WARN << "HttpServer[" << server_.name()
        << "] starts listenning on " << server_.ipPort();
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        conn->setContext(HttpContext());
    }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn,
                            Buffer *buf,
                            Timestamp receiveTime)
{
    HttpContext *context = any_cast<HttpContext>(conn->getMutableContext());

    if (!context->parseRequest(buf, receiveTime))
    {
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }

    if (context->gotAll())
    {
        onRequest(conn, context->request());
        context->reset();
    }
}

void HttpServer::onRequest(const TcpConnectionPtr &conn, const HttpRequest &req)
{
    const string &connection = req.getHeader("Connection");
    bool close = connection == "close" || (req.version() == HttpRequest::kHttp10 && connection != "Keep-Alive");
    HttpResponse response(close);
    httpCallback_(req, &response);
    Buffer buf;
    response.appendToBuffer(&buf);
    conn->send(&buf);
    if (response.closeConnection())
    {
        conn->shutdown();
    }

}



