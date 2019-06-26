/*
 * @Author: py.wang 
 * @Date: 2019-05-31 09:27:08 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-01 07:48:34
 */
#include "src/net/Buffer.h"
#define CATCH_CONFIG_MAIN
#include "src/third/catch.hpp"

using slack::string;
using slack::net::Buffer;

TEST_CASE("Test Buffer Append Retrieve", "[Append Retrieve]")
{
    Buffer buf;
    REQUIRE(buf.readableBytes() == 0);
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize);
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend);

    const string str(200, 'x');
    buf.append(str);
    REQUIRE(buf.readableBytes() == str.size());
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize - str.size());
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend);

    const string str2 = buf.retrieveAsString(50);
    REQUIRE(str2.size() == 50);
    REQUIRE(buf.readableBytes() == str.size() - str2.size());
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize - str.size());
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend + str2.size());
    REQUIRE(str2 == string(50, 'x'));

    buf.append(str);
    REQUIRE(buf.readableBytes() == 2 * str.size() - str2.size());
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize - 2 * str.size());
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend + str2.size());

    const string str3 = buf.retrieveAllAsString();
    REQUIRE(str3.size() == 350);
    REQUIRE(buf.readableBytes() == 0);
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize);
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend);
    REQUIRE(str3 == string(350, 'x'));
}

TEST_CASE("Test Buffer Grow", "[Grow]")
{
    Buffer buf;
    buf.append(string(400, 'y'));
    REQUIRE(buf.readableBytes() == 400);
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize - 400);

    buf.retrieve(50);
    REQUIRE(buf.readableBytes() == 350);
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize - 400);
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend + 50);

    buf.append(string(1000, 'z'));
    REQUIRE(buf.readableBytes() == 1350);
    REQUIRE(buf.writableBytes() == 0);
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend + 50);

    buf.retrieveAll();
    REQUIRE(buf.readableBytes() == 0);
    REQUIRE(buf.writableBytes() == 1400);
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend);
}

TEST_CASE("Test Bufer Inside Grow", "[Inside Grow]")
{
    Buffer buf;
    buf.append(string(800, 'y'));
    REQUIRE(buf.readableBytes() == 800);
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize - 800);

    buf.retrieve(500);
    REQUIRE(buf.readableBytes() == 300);
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize - 800);
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend + 500);

    buf.append(string(300, 'z'));
    REQUIRE(buf.readableBytes() == 600);
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize - 600);
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend);
}

TEST_CASE("Test Buffer Shrink", "[Shrink]")
{
    Buffer buf;
    buf.append(string(2000, 'y'));
    REQUIRE(buf.readableBytes() == 2000);
    REQUIRE(buf.writableBytes() == 0);
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend);

    buf.retrieve(1500);
    REQUIRE(buf.readableBytes() == 500);
    REQUIRE(buf.writableBytes() == 0);
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend + 1500);

    buf.shrink(0);
    REQUIRE(buf.readableBytes() == 500);
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize-500);
    REQUIRE(buf.retrieveAllAsString() == string(500, 'y'));
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend);
}

TEST_CASE("Test Buffer Prepend", "Prepend")
{
    Buffer buf;
    buf.append(string(200, 'y'));
    REQUIRE(buf.readableBytes() == 200);
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize - 200);
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend);

    int x = 0;
    buf.prepend(&x, sizeof x);
    REQUIRE(buf.readableBytes() == 204);
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize - 200);
    REQUIRE(buf.prependableBytes() == Buffer::kCheapPrepend - 4);
}

TEST_CASE("Test Buffer Read Int", "[Read Int]")
{
    Buffer buf;
    buf.append("HTTP");

    REQUIRE(buf.readableBytes() == 4);
    REQUIRE(buf.peekInt8() == 'H');
    int top16 = buf.peekInt16();
    REQUIRE(top16 == 'H'*256 + 'T');
    REQUIRE(buf.peekInt32() == top16*65536 + 'T'*256 + 'P');

    REQUIRE(buf.readInt8() == 'H');
    REQUIRE(buf.readInt16() == 'T'*256 + 'T');
    REQUIRE(buf.readInt8() == 'P');
    REQUIRE(buf.readableBytes() == 0);
    REQUIRE(buf.writableBytes() == Buffer::kInitialSize);

    buf.appendInt8(-1);
    buf.appendInt16(-2);
    buf.appendInt32(-3);
    REQUIRE(buf.readableBytes() == 7);
    REQUIRE(buf.readInt8() == -1);
    REQUIRE(buf.readInt16() == -2);
    REQUIRE(buf.readInt32() == -3);
}

TEST_CASE("Test Buffer Find EOL", "[Find EOL]")
{
    Buffer buf;
    buf.append(string(100000, 'x'));
    const char *null = nullptr;
    REQUIRE(buf.findEOL() == null);
    REQUIRE(buf.findEOL(buf.peek()+90000) == null);
}

void output(Buffer &&buf, const void *inner)
{
    Buffer newBuf(std::move(buf));
    REQUIRE(inner == newBuf.peek());
}

TEST_CASE("Test Move", "[Move]")
{
    Buffer buf;
    buf.append("slack", 5);
    const void *inner = buf.peek();
    output(std::move(buf), inner);
}



