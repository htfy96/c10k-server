#include <c10k/c10k.hpp>

#include "catch.hpp"

TEST_CASE("Plus should be computed", "[plus]")
{
    REQUIRE( plus(1, 2) == 3 );
    REQUIRE( plus(2, -1) == 1);
}
