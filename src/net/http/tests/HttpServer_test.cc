/*
 * @Author: py.wang 
 * @Date: 2019-07-22 08:03:08 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-22 08:42:27
 */
#include "src/net/http/HttpServer.h"
#include "src/net/http/HttpRequest.h"
#include "src/net/http/HttpResponse.h"
#include "src/net/EventLoop.h"
#include "src/log/Logging.h"

#include <iostream>
#include <map>

using namespace slack;
using namespace slack::net;

void onReqeust(const HttpRequest &req, HttpResponse *resp)
{
    std::cout << "Headers " << req.methodString() << " " << req.path() << std::endl;
    const std::map<string, string> &headers = req.headers();
    for (const auto &header : headers)
    {
        std::cout << header.first << ": " << header.second << std::endl;
    }
    if (req.path() == "/")
    {
        LOG_DEBUG << "Home page";
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/html");
        resp->addHeader("Server", "Slack");
        string now = Timestamp::now().toFormattedString();
        resp->setBody("<html><head><title>This is just a test!</title></head>"
            "<body><h1>Hello</h1> Now is " + now + "</body></html>");
    }
    else if (req.path() == "/hello")
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/plain");
        resp->addHeader("Server", "Slack");
        resp->setBody("hello world\n");
    }
    else 
    {
        resp->setStatusCode(HttpResponse::k404NotFound);
        resp->setStatusMessage("Not found");
        resp->setCloseConnection(true);
    }
}

int main(int argc, char *argv[])
{
    int numThreads = 0;
    if (argc > 1)
    {
        numThreads = atoi(argv[1]);
    }
    EventLoop loop;
    HttpServer server(&loop, InetAddress(8000), "dummy");
    server.setHttpCallback(onReqeust);
    server.setThreadNum(numThreads);
    server.start();
    loop.loop();
}