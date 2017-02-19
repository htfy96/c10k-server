//
// Created by lz on 1/25/17.
//

#include <catch.hpp>
#include <c10k/worker_thread.hpp>
#include <c10k/utils.hpp>
#include <c10k/handler.hpp>
#include "test_common.hpp"
#include <cstdint>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <execinfo.h>
#include <unistd.h>

#include "handler/pingpong_handler.hpp"
#include "worker/worker.hpp"


TEST_CASE("multiple WorkerThread test", "[worker_thread]")
{
    using namespace std::chrono_literals;
    using namespace c10k;
    spdlog::set_level(spdlog::level::debug);
    auto server_logger = spdlog::stdout_color_mt("Server"), client_logger = spdlog::stdout_color_mt("Client"),
        debug_logger = spdlog::stdout_color_mt("DEBUG"), acceptor_logger(spdlog::stdout_color_mt("Acceptor"));
    detail::WorkerThread<ServerHandler> worker(400, server_logger);

    std::thread server_t {std::ref(worker)};
    server_t.detach();
    static constexpr int TEST_CNT = 500;


    Acceptor accp("127.0.0.1", 6503, TEST_CNT, [&](int fd) { worker.add_new_connection(fd); }, acceptor_logger);

    std::thread main_t {accp};
    main_t.detach();

    debug_logger->info("Main thread detached");
    cur_sleep_for(3s);
    debug_logger->info("Start to create clientWorker");

    detail::WorkerThread<ClientHandler> clientWorker(400, client_logger);
    std::thread client_t {std::ref(clientWorker)};
    client_t.detach();
    debug_logger->info("ClientWorker created");

    Connector connector("127.0.0.1", 6503, TEST_CNT, [&](int fd) { clientWorker.add_new_connection(fd); }, client_logger);
    connector();


    cur_sleep_for(25s);
    worker.stop(); clientWorker.stop();
    cur_sleep_for(3s);
    INFO(ss.str());
    REQUIRE(check_ok);
}