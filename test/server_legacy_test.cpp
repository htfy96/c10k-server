//
// Created by lz on 3/1/17.
//

#include <catch.hpp>
#include "test_common.hpp"
#include "handler/pingpong_handler.hpp"
#include "worker/worker.hpp"

#include <c10k/server_legacy.hpp>
#include <chrono>

TEST_CASE("ServerLegacyTest", "[server][server_legacy]")
{
    using namespace c10k;
    using namespace std::chrono_literals;
    spdlog::set_level(spdlog::level::debug);

    auto server_t = std::thread([&] {
        auto server_logger = spdlog::stdout_color_mt("Server");
        ServerLegacy<ServerHandler> server(server_logger, 4);
        server.listen(SocketAddress("127.0.0.1", 6506));
    });
    server_t.detach();
    cur_sleep_for(1s);

    constexpr int TEST_CNT = 500;

    auto client_logger = spdlog::stdout_color_mt("Client");
    detail::WorkerThread<ClientHandler> client_worker(1024, client_logger);
    std::thread client_t(std::ref(client_worker));
    client_t.detach();

    cur_sleep_for(1s);

    Connector connector("127.0.0.1", 6506, TEST_CNT, [&](int fd) {
        client_worker.add_new_connection(fd);
    }, client_logger);

    connector();
    cur_sleep_for(8s);

    client_worker.stop();
    cur_sleep_for(1s);

}