//
// Created by lz on 2/19/17.
//

#ifndef C10K_SERVER_WORKER_HPP
#define C10K_SERVER_WORKER_HPP
#include <string>
#include <memory>
#include <functional>
#include <spdlog/spdlog.h>
#include "../test_common.hpp"

class Acceptor
{
private:
    std::string addr;
    int port;
    std::shared_ptr<spdlog::logger> logger;
    using LoggerT = decltype(logger);
    const int TEST_CNT;
    std::function<void(int)> new_conn_handler;
public:
    Acceptor(const std::string &addr, int port, int test_cnt,
             const std::function<void(int)> &new_conn_handler,
             const LoggerT &logger):
            addr(addr), port(port), TEST_CNT(test_cnt), new_conn_handler(new_conn_handler), logger(logger)
    {}

    void operator() ()
    {
        using c10k::detail::call_must_ok;
        int main_fd = create_socket();
        sockaddr_in addr = create_addr("127.0.0.1", 6503);
        call_must_ok(::bind, "Bind", main_fd, (sockaddr*)&addr, sizeof(addr));
        call_must_ok(::listen, "Listen", main_fd, 1024);
        for (int i=0; i<TEST_CNT; ++i)
        {
            int new_sock = call_must_ok(accept, "Accept", main_fd, nullptr, nullptr);
            make_socket_nonblocking(new_sock);
            logger->info("new fd created = {}", new_sock);
            new_conn_handler(new_sock);
        }
        call_must_ok(::close, "close", main_fd);
    }
};

class Connector
{
private:
    std::string addr;
    int port;
    std::shared_ptr<spdlog::logger> logger;
    using LoggerT = decltype(logger);
    const int TEST_CNT;
    std::function<void(int)> new_conn_handler;
public:

    Connector(const std::string &addr, int port, int test_cnt,
              const std::function<void(int)> &new_conn_handler,
              const LoggerT &logger):
            addr(addr), port(port), TEST_CNT(test_cnt), new_conn_handler(new_conn_handler), logger(logger)
    {}

    void operator() ()
    {
        using c10k::detail::call_must_ok;
        try {
            for (int i = 0; i < TEST_CNT; ++i) {
                int cli_fd = create_socket(true);
                sockaddr_in addr = create_addr("127.0.0.1", 6503);
                call_must_ok(::connect, "Connect", cli_fd, (sockaddr *) &addr, sizeof(addr));
                logger->info("New fd added {}: {}/{}", cli_fd, i, TEST_CNT);
                make_socket_nonblocking(cli_fd);
                new_conn_handler(cli_fd);
            }
        } catch(const std::exception &e)
        {
            logger->critical("Critical exception throw: {}", e.what());
            void *bt[40];
            int size = ::backtrace(bt, 40);
            ::backtrace_symbols_fd(bt, size, STDERR_FILENO);
            throw e;
        }
    }

};

#endif //C10K_SERVER_WORKER_HPP
