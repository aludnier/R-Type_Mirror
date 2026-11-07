#include <catch2/catch_test_macros.hpp>

#include "time/FixedStep.h"

TEST_CASE("FixedStep runs no update before a full step", "[time]")
{
    RType::FixedStep fixedStep(0.5);

    REQUIRE(fixedStep.advance(0.25) == 0);
    REQUIRE(fixedStep.advance(0.25) == 1);
}

TEST_CASE("FixedStep keeps the time left over between frames", "[time]")
{
    RType::FixedStep fixedStep(0.5);

    REQUIRE(fixedStep.advance(1.25) == 2);
    REQUIRE(fixedStep.advance(0.25) == 1);
}

TEST_CASE("FixedStep limits the updates after a long freeze", "[time]")
{
    RType::FixedStep fixedStep(0.5, 5);

    REQUIRE(fixedStep.advance(100.0) == 5);
    REQUIRE(fixedStep.advance(0.25) == 0);
}
