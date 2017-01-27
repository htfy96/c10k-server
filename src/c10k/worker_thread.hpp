//
// Created by lz on 1/24/17.
//

#ifndef C10K_SERVER_WORKER_THREAD_HPP
#define C10K_SERVER_WORKER_THREAD_HPP

#include "event_loop.hpp"
#include "connection.hpp"
#include "handler.hpp"
#include <spdlog/spdlog.h>
#include <memory>
#include <type_traits>
#include <functional>

namespace c10k
{
    namespace detail
    {
        // WorkerThread
        template<typename HandlerT>
        class WorkerThread
        {
        private:
            EventLoop loop;
            std::shared_ptr<spdlog::logger> logger;
            using LoggerT = decltype(logger);

        public:
            WorkerThread(int max_event, const LoggerT& logger):
                    loop(max_event, logger), logger(logger)
            {
                logger->info("A new worker thread created");
            }

            WorkerThread(const WorkerThread&) = delete;
            WorkerThread &operator=(const WorkerThread &) = delete;

            void operator() ()
            {
                try {
                    loop.loop();
                } catch(std::exception &e)
                {
                    logger->critical("Error occured! e={}", e.what());
                    throw;
                }
            }

            void add_new_connection(int fd)
            {
                logger->debug("Adding new fd={}", fd);
                auto new_conn = Connection::create(fd, loop, logger, false);
                new_conn->register_event();
                logger->trace("Event registered for fd={}", fd);
                auto handler = std::make_shared<HandlerT>();
                logger->trace("Executing handler");
                handler->handle_init(new_conn);
            }

            std::size_t active_connection_num() const
            {
                return loop.fd_num();
            }

            void stop()
            {
                loop.disable_loop();
            }
        };
    }
}
#endif //C10K_SERVER_WORKER_THREAD_HPP
