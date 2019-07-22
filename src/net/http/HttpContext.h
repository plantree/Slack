/*
 * @Author: py.wang 
 * @Date: 2019-07-21 08:15:13 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-21 08:45:41
 */
#ifndef NET_HTTP_HTTPCONTEXT_H
#define NET_HTTP_HTTPCONTEXT_H

#include "src/base/copyable.h"
#include "src/net/http/HttpRequest.h"

namespace slack
{

namespace net
{

class Buffer;

class HttpContext : public copyable 
{
public:
    // 有限状态机
    enum HttpRequestParseState
    {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll
    };

    HttpContext()
        : state_(kExpectRequestLine)
    {
    }

    // default copy-ctor, dtor and assignment are ok

    bool parseRequest(Buffer *buf, Timestamp receiveTime);

    bool gotAll() const 
    {
        return state_ == kGotAll;
    }

    void reset()
    {
        state_ = kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    const HttpRequest &request() const 
    {
        return request_;
    }

    HttpRequest &request()
    {
        return request_;
    }

private:
    bool processRequestLine(const char *begin, const char *end);

    HttpRequestParseState state_;
    HttpRequest request_;
};

}   // namespace net

}   // namespace slack

#endif  // NET_HTTP_HTTPCONTEXT_H