#include <c10k/c10k.hpp>

#include "catch.hpp"
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>

TEST_CASE("Eventloop should start and stop", "[event_loop]")
{
    using namespace c10k;
    spdlog::set_level(spdlog::level::debug);

    EventLoop el(512);
    std::thread t {[&el]() {
        el.loop();
    }};

    t.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Enable loop
    REQUIRE(el.loop_enabled());
    REQUIRE(el.in_loop());

    // Disable loop
    el.disable_loop();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(!el.in_loop());
    REQUIRE(!el.loop_enabled());

    // Restart loop in another thread
    el.enable_loop();
    std::thread t2 { [&el] {
        el.loop();
    }};
    t2.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(el.loop_enabled());
    REQUIRE(el.in_loop());


    // finally let t2 stop
    el.disable_loop();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST_CASE("Eventloop should work for socket", "[event_loop]")
{
    // TODO
}