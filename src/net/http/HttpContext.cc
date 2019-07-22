/*
 * @Author: py.wang 
 * @Date: 2019-07-21 08:21:33 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-07-22 08:42:22
 */
#include "src/net/Buffer.h"
#include "src/net/http/HttpContext.h"

using namespace slack;
using namespace slack::net;

// 解析请求行
bool HttpContext::processRequestLine(const char *begin, const char *end)
{
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');
    // find method
    if (space != end && request_.setMethod(start, space))
    {
        start = space + 1;
        space = std::find(start, end, ' ');
        // find path
        if (space != end)
        {
            const char *question = std::find(start, space, '?');
            // find query
            if (question != space)
            {
                request_.setPath(start, question);
                request_.setQuery(question, space);
            }
            else 
            {
                request_.setPath(start, space);
            }
            start = space + 1;
            succeed = (end - start == 8) && std::equal(start, end-1, "HTTP/1.");
            if (succeed)
            {
                if (*(end-1) == '1')
                {
                    request_.setVersion(HttpRequest::kHttp11);
                }
                else if (*(end-1) == '0')
                {
                    request_.setVersion(HttpRequest::kHttp10);
                }
                else 
                {
                    succeed = false;
                }
            }
        }
    }          
    return succeed;
}

bool HttpContext::parseRequest(Buffer *buf, Timestamp receiveTime)
{
    bool ok = true;
    bool hasMore = true;
    while (hasMore)
    {
        if (state_ == kExpectRequestLine)
        {
            const char *crlf = buf->findCRLF();
            if (crlf)
            {
                // 解析请求行
                ok = processRequestLine(buf->peek(), crlf);
                if (ok)
                {
                    request_.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf+2);
                    state_ = kExpectHeaders;
                }
                else 
                {
                    hasMore = false;
                }
            }
            else 
            {
                hasMore = false;
            }
        }
        else if (state_ == kExpectHeaders)
        {
            const char *crlf = buf->findCRLF();
            if (crlf)
            {
                const char *colon = std::find(buf->peek(), crlf, ':');
                if (colon != crlf)
                {
                    request_.addHeader(buf->peek(), colon, crlf);
                }
                else 
                {
                    // empty line, end of header
                    state_ = kGotAll;
                    hasMore = false;
                }
                buf->retrieveUntil(crlf+2);
            }
            else 
            {
                hasMore = false;
            }
        }
        else if (state_ == kExpectBody)
        {

        }
    }
    return ok;
}