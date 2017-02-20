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
            RoundRobinPool(const LoggerT &logger):
                logger(logger)
            {}
            RoundRobinPool(const RoundRobinPool &) = delete;
            RoundRobinPool &operator=(const RoundRobinPool &) = delete;

            // new worker will start working immediately
            // not thread-safe. don't call this along with addConnection
            void addWorker(WorkerPtr &&workerPtr)
            {
                workers.push_back(std::move(workerPtr));
                threads.emplace_back(std::ref(**workers.rbegin()));
            }

            int getThreadNum() const
            {
                return threads.size();
            }

            // fd must be readable and non-blocking
            void addConnection(int fd)
            {
                int cnt = round_robin.fetch_add(1);
                logger->trace("Round robin = {}, distributed to thread {}", cnt, cnt % getThreadNum());
                workers[cnt % getThreadNum()]->add_new_connection(fd);
            }

            // stopAll. Make sure you have some thread calling .join()!
            void stopAll()
            {
                logger->info("StopAll() called on WorkerThreadPool");
                std::for_each(workers.begin(), workers.end(), [](WorkerPtr &worker) {
                    worker->stop();
                });
            }

            void join()
            {
                logger->debug("Joining threads...");
                std::for_each(threads.begin(), threads.end(), [&](std::thread &t) {
                    if (t.joinable())
                        t.join();
                    logger->info("a thread has finished!");
                });
            }
        };
    }
}

#endif //C10K_SERVER_ROUND_ROBIN_POOL_HPP
