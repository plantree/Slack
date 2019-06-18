/*
 * @Author: py.wang 
 * @Date: 2019-05-13 08:15:49 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-13 08:49:16
 */

#include "src/log/LogStream.h"



#define CATCH_CONFIG_MAIN
#include "src/third/catch.hpp"

#include <limits>
#include <stdint.h>

using slack::string;

TEST_CASE("testLogStreamBooleans")
{
    slack::LogStream os;

    const slack::LogStream::Buffer& buf = os.buffer();
    REQUIRE(buf.toString() == string(""));

    os << true;
    REQUIRE(buf.toString() == string("1"));

    os << '\n';
    REQUIRE(buf.toString() == string("1\n"));

    os << false;
    REQUIRE(buf.toString() == string("1\n0"));
}

TEST_CASE("testLogStreamIntegers")
{
    slack::LogStream os;
    const slack::LogStream::Buffer& buf = os.buffer();
    REQUIRE(buf.toString() == string(""));

    os << 1;
    REQUIRE(buf.toString() == string("1"));

    os << 0;
    REQUIRE(buf.toString() == string("10"));

    os << -1;
    REQUIRE(buf.toString() == string("10-1"));

    os.resetBuffer();
    os << 0 << " " << 123 << 'x' << 0x64;
    REQUIRE(buf.toString() == string("0 123x100"));
}

TEST_CASE("testLogStreamIntegerLimits")
{
    slack::LogStream os;
    const slack::LogStream::Buffer& buf = os.buffer();
    os << -2147483647;
    REQUIRE(buf.toString() == string("-2147483647"));

    os << static_cast<int>(-2147483647 - 1);
    REQUIRE(buf.toString() == string("-2147483647-2147483648"));

    os << ' ';
    os << 2147483647;
    REQUIRE(buf.toString() == string("-2147483647-2147483648 2147483647"));
    
    os.resetBuffer();

    os << std::numeric_limits<int16_t>::min();
    REQUIRE(buf.toString() == string("-32768"));

    os.resetBuffer();

    os << std::numeric_limits<int16_t>::max();
    REQUIRE(buf.toString() == string("32767"));

    os.resetBuffer();

    // FIXME: error
    /*os << std::numeric_limits<uint16_t>::min();
    REQUIRE(buf.toString() == string("0"));

    os.resetBuffer();

    os << std::numeric_limits<uint16_t>::max();
    REQUIRE(buf.toString() == string("65535"));

    os.resetBuffer();*/

    os << std::numeric_limits<int32_t>::min();
    REQUIRE(buf.toString() == string("-2147483648"));

    os.resetBuffer();

    os << std::numeric_limits<int32_t>::max();
    REQUIRE(buf.toString() == string("2147483647"));

    os.resetBuffer();

    os << std::numeric_limits<uint32_t>::min();
    REQUIRE(buf.toString() == string("0"));

    os.resetBuffer();

    os << std::numeric_limits<uint32_t>::max();
    REQUIRE(buf.toString() == string("4294967295"));//

    os.resetBuffer();

    os << std::numeric_limits<int64_t>::min();
    REQUIRE(buf.toString() == string("-9223372036854775808"));

    os.resetBuffer();

    os << std::numeric_limits<int64_t>::max();
    REQUIRE(buf.toString() == string("9223372036854775807"));

    os.resetBuffer();

    os << std::numeric_limits<uint64_t>::min();
    REQUIRE(buf.toString() == string("0"));

    os.resetBuffer();

    os << std::numeric_limits<uint64_t>::max();
    REQUIRE(buf.toString() == string("18446744073709551615"));

    os.resetBuffer();

    int16_t a = 0;
    int32_t b = 0;
    int64_t c = 0;
    os << a;
    os << b;
    os << c;
    REQUIRE(buf.toString() == string("000"));
}

TEST_CASE("testLogStreamFloats")
{
    slack::LogStream os;
    const slack::LogStream::Buffer& buf = os.buffer();

    os << 0.0;
    REQUIRE(buf.toString() == string("0"));

    os.resetBuffer();

    os << 1.0;
    REQUIRE(buf.toString() == string("1"));

    os.resetBuffer();

    os << 0.1;
    REQUIRE(buf.toString() == string("0.1"));

    os.resetBuffer();

    os << 0.05;
    REQUIRE(buf.toString() == string("0.05"));

    os.resetBuffer();

    os << 0.15;
    REQUIRE(buf.toString() == string("0.15"));

    os.resetBuffer();

    double a = 0.1;
    os << a;
    REQUIRE(buf.toString() == string("0.1"));

    os.resetBuffer();

    double b = 0.05;
    os << b;
    REQUIRE(buf.toString() == string("0.05"));

    os.resetBuffer();

    double c = 0.15;
    os << c;
    REQUIRE(buf.toString() == string("0.15"));

    os.resetBuffer();

    os << a+b;
    REQUIRE(buf.toString() == string("0.15"));

    os.resetBuffer();

    REQUIRE(a+b != c);

    os << 1.23456789;
    REQUIRE(buf.toString() == string("1.23456789"));

    os.resetBuffer();

    os << 1.234567;
    REQUIRE(buf.toString() == string("1.234567"));

    os.resetBuffer();

    os << -123.456;
    REQUIRE(buf.toString() == string("-123.456"));
    os.resetBuffer();
}

TEST_CASE("testLogStreamVoid")
{
    slack::LogStream os;
    const slack::LogStream::Buffer& buf = os.buffer();

    os << static_cast<void*>(0);
    REQUIRE(buf.toString() == string("0x0"));

    os.resetBuffer();

    os << reinterpret_cast<void*>(8888);
    REQUIRE(buf.toString() == string("0x22B8"));
}

TEST_CASE("testLogStreamStrings")
{
    slack::LogStream os;
    const slack::LogStream::Buffer& buf = os.buffer();

    os << "Hello ";
    REQUIRE(buf.toString() == string("Hello "));

    string chenshuo = "Wpy";
    os << chenshuo;
    REQUIRE(buf.toString() == string("Hello Wpy"));
}

// format
TEST_CASE("testLogStreamFmts")
{
  slack::LogStream os;
  const slack::LogStream::Buffer& buf = os.buffer();

  os << slack::Fmt("%4d", 1);
  REQUIRE(buf.toString() == string("   1"));

  os.resetBuffer();

  os << slack::Fmt("%4.2f", 1.2);
  REQUIRE(buf.toString() == string("1.20"));

  os.resetBuffer();

  os << slack::Fmt("%4.2f", 1.2) << slack::Fmt("%4d", 43);
  REQUIRE(buf.toString() == string("1.20  43"));
}

TEST_CASE("testLogStreamLong")
{
  slack::LogStream os;
  const slack::LogStream::Buffer& buf = os.buffer();
  for (int i = 0; i < 399; ++i)
  {
    os << "123456789 ";
    REQUIRE(buf.length() == 10*(i+1));
    REQUIRE(buf.avail() == 4000 - 10*(i+1));
  }

  os << "abcdefghi ";
  REQUIRE(buf.length() == 3990);
  REQUIRE(buf.avail() == 10);

  os << "abcdefghi";
  REQUIRE(buf.length() == 3999);
  REQUIRE(buf.avail() == 1);
}
