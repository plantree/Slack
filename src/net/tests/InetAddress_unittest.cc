/*
 * @Author: py.wang 
 * @Date: 2019-05-27 07:58:52 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-27 08:27:40
 */

#include "src/net/InetAddress.h"
#include "src/log/Logging.h"

#define CATCH_CONFIG_MAIN
#include "src/third/catch.hpp" // unittest tool

using slack::string;
using slack::net::InetAddress;

TEST_CASE("InetAddress convert", "[InetAddress]")
{
    InetAddress addr0(1234);
    REQUIRE(addr0.toIp() == string("0.0.0.0"));
    REQUIRE(addr0.toIpPort() == string("0.0.0.0:1234"));
    REQUIRE(addr0.toPort() == 1234);

    InetAddress addr1(4321, true);
    REQUIRE(addr1.toIp() == string("127.0.0.1"));
    REQUIRE(addr1.toIpPort() == string("127.0.0.1:4321"));
    REQUIRE(addr1.toPort() == 4321);
    
    InetAddress addr2("1.2.3.4", 8888);
    REQUIRE(addr2.toIp() == string("1.2.3.4"));
    REQUIRE(addr2.toIpPort() == string("1.2.3.4:8888"));
    REQUIRE(addr2.toPort() == 8888);

    InetAddress addr3("255.254.253.252", 65535);
    REQUIRE(addr3.toIp() == string("255.254.253.252"));
    REQUIRE(addr3.toIpPort() == string("255.254.253.252:65535"));
    REQUIRE(addr3.toPort() == 65535);
}

TEST_CASE("Test InetAddress Resolve", "[Resolve]")
{
    InetAddress addr(80);
    if (InetAddress::resolve("google.com", &addr))
    {
        LOG_INFO << "google.com resolved to " << addr.toIpPort();
    }
    else 
    {
        LOG_ERROR << "Unable to resolve google.com";
    }
}