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


TEST_CASE("multiple WorkerThread test", "[worker_thread]")
{
    using namespace std::chrono_literals;
    using namespace c10k;
    using detail::call_must_ok;
    spdlog::set_level(spdlog::level::debug);
    auto server_logger = spdlog::stdout_color_mt("Server"), client_logger = spdlog::stdout_color_mt("Client"),
        debug_logger = spdlog::stdout_color_mt("DEBUG"), acceptor_logger(spdlog::stdout_color_mt("Acceptor"));
    detail::WorkerThread<ServerHandler> worker(400, server_logger);

    std::thread server_t {std::ref(worker)};
    server_t.detach();
    static constexpr int TEST_CNT = 500;

    std::thread main_t {[&]() {
        int main_fd = create_socket();
        sockaddr_in addr = create_addr("127.0.0.1", 6503);
        call_must_ok(::bind, "Bind", main_fd, (sockaddr*)&addr, sizeof(addr));
        call_must_ok(::listen, "Listen", main_fd, 1024);
        for (int i=0; i<TEST_CNT; ++i)
        {
            int new_sock = call_must_ok(accept, "Accept", main_fd, nullptr, nullptr);
            make_socket_nonblocking(new_sock);
            acceptor_logger->info("new fd created = {}", new_sock);
            worker.add_new_connection(new_sock);
        }
        call_must_ok(::close, "close", main_fd);
    }};
    main_t.detach();
    debug_logger->info("Main thread detached");
    cur_sleep_for(3s);
    debug_logger->info("Start to create clientWorker");

    detail::WorkerThread<ClientHandler> clientWorker(400, client_logger);
    std::thread client_t {std::ref(clientWorker)};
    client_t.detach();
    debug_logger->info("ClientWorker created");

    try {
        for (int i = 0; i < TEST_CNT; ++i) {
            int cli_fd = create_socket(true);
            sockaddr_in addr = create_addr("127.0.0.1", 6503);
            call_must_ok(::connect, "Connect", cli_fd, (sockaddr *) &addr, sizeof(addr));
            debug_logger->info("New fd added {}: {}/{}", cli_fd, i, TEST_CNT);
            make_socket_nonblocking(cli_fd);
            clientWorker.add_new_connection(cli_fd);
        }
    } catch(const std::exception &e)
    {
        debug_logger->critical("Critical exception throw: {}", e.what());
        void *bt[40];
        int size = ::backtrace(bt, 40);
        ::backtrace_symbols_fd(bt, size, STDERR_FILENO);
        throw e;
    }

    cur_sleep_for(25s);
    worker.stop(); clientWorker.stop();
    cur_sleep_for(3s);
    INFO(ss.str());
    REQUIRE(check_ok);
}