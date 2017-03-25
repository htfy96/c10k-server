#include <spdlog/logger.h>
#include "worker_thread_pool.hpp"
#include "addr.hpp"
#include "handler.hpp"
#include <memory>

namespace c10k
{
    template<typename HandlerT>
    class ServerLegacy
    {
        static_assert(std::is_base_of<Handler, HandlerT>::value, "HandlerT must derive c10k::Handler interface");
    private:
        std::shared_ptr<spdlog::logger> logger;
        using LoggerT = decltype(logger);
    public:
        ServerLegacy(const LoggerT& logger, int worker_num);
        void listen(const SocketAddress &addr);
    private:
        detail::WorkerThreadPool pool;
    };

    template<typename HandlerT>
    ServerLegacy<HandlerT>::ServerLegacy(const LoggerT &logger, int worker_num):logger(logger), pool(logger)
    {
        using detail::WorkerThread;
        for (int i = 0; i<worker_num; ++i) {
            logger->info("Adding {}-th worker into thread pool", i);
            pool.addWorker(std::make_unique<WorkerThread<HandlerT>>(1024, logger));
        }
    }

    template<typename HandlerT>
    void ServerLegacy<HandlerT>::listen(const SocketAddress &addr)
    {
        using detail::call_must_ok;
        using detail::make_exit_guard;
        int mainsockfd = detail::create_socket(true);
        auto exit_guard = make_exit_guard([mainsockfd]() {
            call_must_ok(::close, "Close", mainsockfd);
        });
        call_must_ok(::bind, "Bind", mainsockfd, (sockaddr*)&addr.addrin(), sizeof(addr.addrin()));
        call_must_ok(::listen, "Listen", mainsockfd, 4096);

        logger->info("Start listening at {}:{}...", addr.ip(), addr.port());
        for (;;)
        {
            int new_sock = call_must_ok(accept, "Accept", mainsockfd, nullptr, nullptr);
            make_socket_nonblocking(new_sock);
            logger->debug("Accept new socket={}", new_sock);
            pool.addConnection(new_sock);
        }
    }
}