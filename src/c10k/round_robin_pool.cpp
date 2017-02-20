//
// Created by lz on 2/20/17.
//

#include "round_robin_pool.hpp"

namespace c10k
{
    namespace detail
    {
        RoundRobinPool::RoundRobinPool(const LoggerT &logger):
                logger(logger)
        {
        }


        void RoundRobinPool::addWorker(WorkerPtr &&workerPtr)
        {
            workers.push_back(std::move(workerPtr));
            threads.emplace_back(std::ref(**workers.rbegin()));
        }

        int RoundRobinPool::getThreadNum() const
        {
            return threads.size();
        }

        void RoundRobinPool::addConnection(int fd)
        {
            int cnt = round_robin.fetch_add(1);
            logger->trace("Round robin = {}, distributed to thread {}", cnt, cnt % getThreadNum());
            workers[cnt % getThreadNum()]->add_new_connection(fd);
        }

        void RoundRobinPool::stopAll()
        {
            logger->info("StopAll() called on WorkerThreadPool");
            std::for_each(workers.begin(), workers.end(), [](WorkerPtr &worker) {
                worker->stop();
            });
        }

        void RoundRobinPool::join()
        {
            logger->debug("Joining threads...");
            std::for_each(threads.begin(), threads.end(), [&](std::thread &t) {
                if (t.joinable())
                    t.join();
                logger->info("a thread has finished!");
            });
        }
    }
}
