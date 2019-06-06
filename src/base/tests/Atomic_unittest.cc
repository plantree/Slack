/*
 * @Author: py.wang 
 * @Date: 2019-06-05 20:17:50 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-05 20:23:56
 */

#include "src/base/Atomic.h"

#define CATCH_CONFIG_MAIN
#include "src/third/catch.hpp"

TEST_CASE("Test Atomic", "[Atomic]")
{
    {
        slack::AtomicInt64 a0;
        REQUIRE(a0.get() == 0);
        REQUIRE(a0.getAndAdd(1) == 0);
        REQUIRE(a0.get() == 1);
        REQUIRE(a0.addAndGet(2) == 3);
        REQUIRE(a0.get() == 3);
        REQUIRE(a0.incrementAndGet() == 4);
        REQUIRE(a0.get() == 4);
        a0.increment();
        REQUIRE(a0.get() == 5);
        REQUIRE(a0.addAndGet(-3) == 2);
        REQUIRE(a0.getAndSet(100) == 2);
        REQUIRE(a0.get() == 100);
    }

    {
        slack::AtomicInt64 a1;
        REQUIRE(a1.get() == 0);
        REQUIRE(a1.getAndAdd(1) == 0);
        REQUIRE(a1.get() == 1);
        REQUIRE(a1.addAndGet(2) == 3);
        REQUIRE(a1.get() == 3);
        REQUIRE(a1.incrementAndGet() == 4);
        REQUIRE(a1.get() == 4);
        a1.increment();
        REQUIRE(a1.get() == 5);
        REQUIRE(a1.addAndGet(-3) == 2);
        REQUIRE(a1.getAndSet(100) == 2);
        REQUIRE(a1.get() == 100);
    }
}