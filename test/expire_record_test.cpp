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

    ExpireRecord<int> er(100ms);

    for (int i=0; i<5; ++i)
    {
        er.push_element(i);
        cur_sleep_for(10ms);
    }

    SECTION("remove_expired are called immediately")
    {
        auto expired = er.get_expired_and_remove();
        REQUIRE(expired.empty());
    }

    SECTION("remove_expired are called after 25ms")
    {
        // 0,   10,     20,     30,     40,     50,         75
        // +0   +1      +2       +3      +4
        cur_sleep_for(25ms);
        auto expired = er.get_expired_and_remove();
        REQUIRE(expired.empty());
    }

    SECTION("remove_expired are called after 65ms")
    {
        // cur_point: 115ms. Element 0, 1 should be removed
        cur_sleep_for(65ms);
        auto expired =er.get_expired_and_remove();
        REQUIRE(expired.size() == 2);
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 0) != expired.cend());
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 1) != expired.cend());
    }

    SECTION("remove_expired are called after 85ms")
    {
        // cur_point: 135ms. Element 0, 1, 2, 3 should be removed
        cur_sleep_for(85ms);
        auto expired =er.get_expired_and_remove();
        REQUIRE(expired.size() == 4);
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 0) != expired.cend());
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 1) != expired.cend());
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 2) != expired.cend());
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 3) != expired.cend());
    }

    SECTION("Add after remove after 75ms. then remove after 50ms") {
        cur_sleep_for(75ms);
        // cur_point: 125ms. Elements 0, 1, 2 should be removed
        auto expired = er.get_expired_and_remove();
        REQUIRE(expired.size() == 3);
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 0) != expired.cend());
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 1) != expired.cend());
        REQUIRE(std::find(expired.cbegin(), expired.cend(), 2) != expired.cend());

        er.push_element(5);
        cur_sleep_for(50ms);
        // cur_point: 175ms. 3, 4 should be removed
        auto new_expired = er.get_expired_and_remove();
        REQUIRE(new_expired.size() == 2);
        REQUIRE(std::find(new_expired.cbegin(), new_expired.cend(), 3) != new_expired.cend());
        REQUIRE(std::find(new_expired.cbegin(), new_expired.cend(), 4) != new_expired.cend());
    }

}