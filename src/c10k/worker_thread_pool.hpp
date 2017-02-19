//
// Created by lz on 2/9/17.
//

#ifndef C10K_SERVER_WORKER_THREAD_POOL_HPP
#define C10K_SERVER_WORKER_THREAD_POOL_HPP

#include <thread>
#include <vector>
#include <mutex>
#include <memory>
#include <stdexcept>
#include <atomic>
#include <spdlog/spdlog.h>
#include "worker_thread.hpp"
#include "queue.hpp"

namespace c10k
{
    template<typename HandlerT>
    class WorkerThreadPool {
        using WorkerPtr = std::unique_ptr<detail::WorkerThread<HandlerT>>;
        std::vector<std::thread> threads;
        std::vector<WorkerPtr> workers;
        std::atomic_int round_robin {0};
        const int max_event_per_loop;
        std::shared_ptr<spdlog::logger> logger;
        using LoggerT = decltype(logger);

    public:
        WorkerThreadPool(int thread_num,
                         int max_event_per_loop,
                         const LoggerT &logger = spdlog::stdout_color_mt("c10k_TPool")):
                max_event_per_loop(max_event_per_loop),
                logger(logger)
        {
            workers.reserve(thread_num);
            threads.reserve(thread_num);
            for (int i=0; i<thread_num; ++i) {
                logger->info("Adding worker {} into WorkerPool", i);
                workers.emplace_back(std::make_unique<detail::WorkerThread<HandlerT>>(max_event_per_loop, logger));
                threads.emplace_back(std::ref(*workers[i]));
            }
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

#endif //C10K_SERVER_WORKER_THREAD_POOL_HPP
