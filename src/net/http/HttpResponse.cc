/*
 * @Author: py.wang 
 * @Date: 2019-07-21 08:53:26 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-22 08:16:02
 */
#include "src/net/http/HttpResponse.h"
#include "src/net/Buffer.h"

#include <stdio.h>

using namespace slack;
using namespace slack::net;

void HttpResponse::appendToBuffer(Buffer *output) const 
{
    char buf[32];
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
    output->append(buf);
    output->append(statusMessage_);
    output->append("\r\n");

    if (closeConnection())
    {
        output->append("Connection: close\r\n");
    }
    else 
    {
        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
        output->append(buf);
        output->append("Connection: Keep-Alive\r\n");
    }

    for (const auto &header : headers_)
    {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }

    output->append("\r\n");
    output->append(body_);
}