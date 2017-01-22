#include <c10k/event_loop.hpp>
#include <c10k/endian.hpp>

#include "catch.hpp"
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <sys/socket.h>
#include <condition_variable>
#include <mutex>
#include <cerrno>
#include <algorithm>
#include <numeric>

auto ev_logger = spdlog::stdout_color_mt("Eventloop");
auto debug_logger = spdlog::stdout_color_mt("debug");
using namespace std::chrono_literals;

inline void cur_sleep_for(std::chrono::milliseconds ms)
{
    std::this_thread::sleep_for(ms);
}

TEST_CASE("Eventloop should start and stop", "[event_loop]")
{
    using namespace c10k;
    spdlog::set_level(spdlog::level::debug);

    EventLoop el(512, ev_logger);
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
    using namespace c10k;
    using detail::call_must_ok;
    spdlog::set_level(spdlog::level::debug);
    auto &logger = debug_logger;

    int socketfd = call_must_ok(socket, "Create socket", AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    call_must_ok(inet_aton, "inet_aton", "127.0.0.1", &addr.sin_addr);
    addr.sin_port = detail::to_net16(6532);

    static const int ENABLE = 1;
    int flags = call_must_ok(fcntl, "F_GETFL", socketfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    call_must_ok(fcntl, "F_SETFL", socketfd, F_SETFL, flags);
    call_must_ok(setsockopt, "setsockopt(REUSEADDR)", socketfd, SOL_SOCKET, SO_REUSEADDR, &ENABLE, sizeof(ENABLE));
    call_must_ok(bind, "bind", socketfd, (sockaddr*)&addr, sizeof(addr));

    c10k::EventLoop el(1024, ev_logger);


    int server_read_cnt = 0;
    int server_read_len = 0;
    char server_buffer[65536];
    memset(server_buffer, 0, sizeof(server_buffer));

    auto close_handler = [&](EventLoop &el, int socketfd) {
        call_must_ok(::close, "close socket", socketfd);
    };

    std::thread server_t([&] {
        call_must_ok(listen, "listen", socketfd, 1024);
        logger->debug("listen successed, socketfd={}", socketfd);

        el.add_event(socketfd, EventType(EventCategory::POLLIN).set(EventCategory::POLLRDHUP), [&](const Event &e) {
            logger->debug("main socket received event {}", e.event_type);
            if (e.event_type.is(EventCategory::POLLRDHUP))
            {
                logger->debug("main received pollrdhup, stopping...");
                el.disable_loop();
                close_handler(el, socketfd);
            }
            else {
                logger->debug("main received pollin, accepting...");
                int accept_socketfd = call_must_ok(::accept, "Accept", socketfd, nullptr, nullptr);
                logger->debug("accepted fd={}", accept_socketfd);
                REQUIRE(accept_socketfd > 0);

                EventType e_in_and_rdhup;
                e_in_and_rdhup.set(EventCategory::POLLIN).set(EventCategory::POLLRDHUP);
                logger->debug("Add event on socket {}", accept_socketfd);
                el.add_event(accept_socketfd, e_in_and_rdhup, [&](const Event &e) {
                    int accept_socketfd = e.fd;
                    logger->debug("Socket {} received event {}", accept_socketfd, e.event_type);
                    if (e.event_type.is(EventCategory::POLLRDHUP) ||
                        e.event_type.is(EventCategory::POLLERR) ||
                        e.event_type.is(EventCategory::POLLHUP)) {
                        logger->debug("Err occured, closing socket {}...", accept_socketfd);
                        el.remove_event(accept_socketfd);
                        close_handler(el, accept_socketfd);
                    } else {
                        int read_result = read(accept_socketfd, server_buffer + server_read_len, 956);
                        logger->debug("Read result = {}, server_read_cnt={}, server_read_len={}, errno={}", read_result,
                        server_read_cnt, server_read_len, errno);
                        if (read_result < 0 && errno != EAGAIN) {
                            close_handler(el, accept_socketfd);
                        }
                        if (read_result > 0) {
                            server_read_len += read_result;
                            server_read_cnt++;
                        }
                    }
                });
            }
        });

        el.loop();
        logger->debug("loop stopped, closing main socket");
        close_handler(el, socketfd);
    });

    server_t.detach();

    cur_sleep_for(50ms);

    SECTION("Should work for single client") {
        int clientsock = call_must_ok(socket, "Create client socket", AF_INET, SOCK_STREAM, 0);
        call_must_ok(setsockopt, "setsockopt(REUSEADDR)", clientsock, SOL_SOCKET, SO_REUSEADDR, &ENABLE, sizeof(ENABLE));
        call_must_ok(connect, "connect", clientsock, (sockaddr*)&addr, sizeof(addr));

        char local_buf[sizeof(server_buffer)];
        std::generate(local_buf, local_buf + sizeof(local_buf), []() {
            return 'A' + std::rand() % 26;
        });

        for (char *cur_pos = local_buf; cur_pos != local_buf + sizeof(local_buf);)
        {
            int write_len = call_must_ok(::write, "Write", clientsock, cur_pos, 1024);
            logger->debug("Written {} from clientsock from pos={}", write_len, cur_pos - local_buf);
            cur_pos += write_len;
        }

        cur_sleep_for(200ms);
        logger->debug("closing clientsocket");
        ::close(clientsock);

        cur_sleep_for(200ms);
        logger->debug("Validating result");
        for (int i=0; i<sizeof(local_buf); ++i)
            REQUIRE(server_buffer[i] == local_buf[i]);

        el.disable_loop();
        cur_sleep_for(200ms);
    }
}