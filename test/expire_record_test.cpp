//
// Created by lz on 2/4/17.
//

#include <catch.hpp>
#include <vector>
#include <chrono>
#include <c10k/expire_record.hpp>
#include "test_common.hpp"

TEST_CASE("ExpireRecord should handle various conditions", "[ExpireRecord]")
{
    using c10k::detail::ExpireRecord;
    using namespace std::chrono_literals;

    ExpireRecord<int> er(200ms);

    for (int i=0; i<5; ++i)
    {
        er.push_element(i);
        cur_sleep_for(20ms);
    }

    SECTION("remove_expired are called immediately")
    {
        auto expired = er.get_expired_and_remove();
        REQUIRE(expired.empty());
    }

    SECTION("remove_expired are called after 50ms")
    {
        // 0,   10,     20,     30,     40,     50,         75
        // +0   +1      +2       +3      +4
        cur_sleep_for(50ms);
        // cur_point: 150ms
        auto expired = er.get_expired_and_remove();
        REQUIRE(expired.empty());
    }

    SECTION("remove_expired are called after 130ms")
    {
        // cur_point: 230ms. Element 0, 1 should be removed
        cur_sleep_for(130ms);
        auto expired =er.get_expired_and_remove();
        REQUIRE(expired.size() == 2);
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 0) != expired.cend());
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 1) != expired.cend());
    }

    SECTION("remove_expired are called after 170ms")
    {
        // cur_point: 270ms. Element 0, 1, 2, 3 should be removed
        cur_sleep_for(170ms);
        auto expired =er.get_expired_and_remove();
        REQUIRE(expired.size() == 4);
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 0) != expired.cend());
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 1) != expired.cend());
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 2) != expired.cend());
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 3) != expired.cend());
    }

    SECTION("Add after remove after 150ms. then remove after 100ms") {
        cur_sleep_for(150ms);
        // cur_point: 250ms. Elements 0, 1, 2 should be removed
        auto expired = er.get_expired_and_remove();
        REQUIRE(expired.size() == 3);
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 0) != expired.cend());
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 1) != expired.cend());
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 2) != expired.cend());

        er.push_element(5);
        cur_sleep_for(100ms);
        // cur_point: 350ms. 3, 4 should be removed
        auto new_expired = er.get_expired_and_remove();
        REQUIRE(new_expired.size() == 2);
        REQUIRE(std::find(new_expired.cbegin(), new_expired.cend(), 3) != new_expired.cend());
        REQUIRE(std::find(new_expired.cbegin(), new_expired.cend(), 4) != new_expired.cend());
    }

}