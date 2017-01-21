//
// Created by lz on 1/21/17.
//

#include <catch.hpp>
#include <c10k/endian.hpp>

TEST_CASE("Test Endian Conversion")
{
    using c10k::detail::to_host32;
    using c10k::detail::to_host16;

    using c10k::detail::to_net32;
    using c10k::detail::to_net16;

    REQUIRE(to_net32(0x12345678) == 0x78563412);
    REQUIRE(to_net16(0x1234) == 0x3412);
    REQUIRE(to_host16(0x1234) == 0x3412);
    REQUIRE(to_host32(0x12345678) == 0x78563412);
}
