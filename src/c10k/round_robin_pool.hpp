//
// Created by lz on 2/20/17.
//

#ifndef C10K_SERVER_ROUND_ROBIN_POOL_HPP
#define C10K_SERVER_ROUND_ROBIN_POOL_HPP
#include <vector>
#include <memory>
#include <atomic>
#include <spdlog/spdlog.h>
#include "utils.hpp"
#include "worker_thread.hpp"

namespace c10k
{
    namespace detail
    {
        using WorkerPtr = std::unique_ptr<WorkerThreadInterface>;
        class RoundRobinPool {

            std::vector<std::thread> threads;
            std::vector<WorkerPtr> workers;
            std::atomic_int round_robin {0};
            std::shared_ptr<spdlog::logger> logger;
            using LoggerT = decltype(logger);

        public:
            RoundRobinPool(const LoggerT &logger);
            RoundRobinPool(const RoundRobinPool &) = delete;
            RoundRobinPool &operator=(const RoundRobinPool &) = delete;

            // new worker will start working immediately
            // not thread-safe. don't call this along with addConnection
            void addWorker(WorkerPtr &&workerPtr);

            int getThreadNum() const;

            // fd must be readable and non-blocking
            void addConnection(int fd);

            // stopAll. Make sure you have some thread calling .join()!
            void stopAll();

            void join();
        };
    }
}

#endif //C10K_SERVER_ROUND_ROBIN_POOL_HPP
