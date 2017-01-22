//
// Created by lz on 1/22/17.
//

#include <c10k/queue.hpp>
#include <catch.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <cstddef>
#include <chrono>

using namespace std::chrono_literals;

TEST_CASE("Queue in multi producer multi consumer status", "[queue]")
{
    using namespace c10k;
    detail::BoundedBlockingQueue<int> bbq(15);

    std::atomic_int producer_cnt {0}, consumer_cnt {0};
    std::atomic_bool stop_producer {false}, stop_consumer {false};

    constexpr std::size_t PRODUCER_CNT = 10, CONSUMER_CNT = 8;

    std::vector<std::thread> producers, consumers;
    for (int i=0; i<PRODUCER_CNT; ++i) {
        producers.emplace_back([&]() {
            while (!stop_producer.load()) {
                bbq.push(std::rand() % 25);
                producer_cnt.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::microseconds(std::rand() % 10));
            }
        });
        producers.rbegin()->detach();
    }

    for (int i=0; i<CONSUMER_CNT; ++i) {
        consumers.emplace_back([&]() {
            while (!stop_consumer.load()) {
                int result;
                bbq.pop(result);
                consumer_cnt.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::microseconds(std::rand() % 10));
            }
        });
        consumers.rbegin()->detach();
    }

    std::this_thread::sleep_for(200ms);
    stop_producer.store(true);

    std::this_thread::sleep_for(50ms);
    stop_consumer.store(true);

    std::this_thread::sleep_for(50ms);
    REQUIRE(producer_cnt.load() == consumer_cnt.load());
    REQUIRE(bbq.is_empty());

}