//
// Created by lz on 1/24/17.
//

#include <catch.hpp>
#include <c10k/connection.hpp>
#include <c10k/event_loop.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <thread>
#include <chrono>
#include "test_common.hpp"

std::shared_ptr<spdlog::logger> server_logger = spdlog::stdout_color_mt("Server"),
client_logger = spdlog::stdout_color_mt("Client");

using LoggerT = decltype(server_logger);

class ConnectionTester1
{
public:
    std::vector<char> v;
    void operator() (c10k::EventLoop &el, LoggerT logger)
    {
        using namespace c10k;
        using c10k::detail::call_must_ok;
        sockaddr_in addr = create_addr("127.0.0.1", 6502);
        int sock = create_socket(false); // nonblocking
        call_must_ok(bind, "bind", sock, (sockaddr*)&addr, sizeof(addr));
        call_must_ok(listen, "listen", sock, 1024);

        int acc_fd = -1;
        for (;acc_fd < 0;) {
            acc_fd = ::accept(sock, nullptr, nullptr);
        }
        auto conn = Connection::create(acc_fd, el, logger, false);
        conn->register_event();

        conn->read_async_then(std::back_inserter(v), 2, [&](const ConnectionPtr &conn, char *st, char *ed) {
            int len = (unsigned)st[0] * 128 + (unsigned)st[1];
            conn->read_async_then(std::back_inserter(v), len, [&](const ConnectionPtr &conn, char *st, char *ed) {
                conn->close();
            });
        });

        el.loop();
    }
};

class ConnectionTester2
{
public:
    std::vector<char> v;
    void gen_data()
    {
        std::uint16_t len = std::rand() % 16384;
        v.push_back(len / 128);
        v.push_back(len % 128);
        std::generate_n(std::back_inserter(v), len, []() {
            return std::rand() % 128;
        });
    }

    void operator() (c10k::EventLoop &el, LoggerT logger)
    {
        using namespace c10k;
        using c10k::detail::call_must_ok;
        sockaddr_in addr = create_addr("127.0.0.1", 6502);
        int sock = create_socket(true); // blocking before connect

        call_must_ok(connect, "connect", sock, (sockaddr*)&addr, sizeof(addr));
        make_socket_nonblocking(sock);
        auto conn = Connection::create(sock, el, logger, false);
        conn->register_event();

        conn->write_async_then(v.begin(), v.begin() + 2, [&](const ConnectionPtr &conn) {
            logger->info("Write from v.data+2 {} to v.data()+v.size() {}", (void*)v.data(), (void*)(v.data() + v.size()));
            conn->write_async_then(v.data() + 2, v.data() + v.size(), [&](const ConnectionPtr &conn) {
                logger->info("data written");
                conn->close();
            });
        });
        el.loop();
    }
};

using namespace std::chrono_literals;
TEST_CASE("Connection should handle packet corrected", "[connection]")
{
    spdlog::set_level(spdlog::level::debug);
    using namespace c10k;
    using c10k::detail::call_must_ok;
    EventLoop server_loop(400, server_logger), client_loop(400, client_logger);


    ConnectionTester1 server; ConnectionTester2 client; client.gen_data();
    std::thread server_t([&]() {
        server(server_loop, server_logger);
    });
    server_t.detach();
    cur_sleep_for(200ms);

    std::thread client_t([&]() {
        client(client_loop, client_logger);
    });
    client_t.detach();

    cur_sleep_for(500ms);
    {
        REQUIRE(server.v.size() == client.v.size());
        for (int i = 0; i < client.v.size(); ++i)
            REQUIRE(server.v[i] == client.v[i]);
    }

    client_loop.disable_loop();
    server_loop.disable_loop();
    cur_sleep_for(200ms);
}