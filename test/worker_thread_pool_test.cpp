//
// Created by lz on 2/19/17.
//

#include <catch.hpp>
#include "test_common.hpp"
#include "worker/worker.hpp"
#include "handler/pingpong_handler.hpp"
#include <spdlog/spdlog.h>

#include <c10k/worker_thread_pool.hpp>

TEST_CASE("WorkerThreadPool should work for multiple threads", "[worker_thread][thread_pool]")
{
    using namespace std::chrono_literals;
    using namespace c10k;
    using detail::call_must_ok;
    spdlog::set_level(spdlog::level::debug);
    auto server_logger = spdlog::stdout_color_mt("Server"), client_logger = spdlog::stdout_color_mt("Client"),
            debug_logger = spdlog::stdout_color_mt("DEBUG"), acceptor_logger(spdlog::stdout_color_mt("Acceptor"));

    detail::WorkerThreadPool pool(server_logger);
    for (int i=0; i<4; ++i)
        pool.addWorker(std::make_unique<detail::WorkerThread<ServerHandler>>(1024, server_logger));

    std::thread server_t([&]() {
        pool.join();
    });
    server_t.detach();


    static constexpr int TEST_CNT = 500;
    Acceptor acceptor("127.0.0.1", 6504, TEST_CNT, [&](int fd) {
        pool.addConnection(fd);
    }, acceptor_logger);

    std::thread accept_t(std::ref(acceptor));
    accept_t.detach();

    detail::WorkerThread<ClientHandler> client_worker(1024, client_logger);
    std::thread client_t(std::ref(client_worker));
    client_t.detach();

    cur_sleep_for(1s);

    Connector connector("127.0.0.1", 6504, TEST_CNT, [&](int fd) {
        client_worker.add_new_connection(fd);
    }, client_logger);


    connector();
    cur_sleep_for(8s);
    pool.stopAll();
    client_worker.stop();

    cur_sleep_for(3s);

}