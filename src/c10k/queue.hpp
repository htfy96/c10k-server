//
// Created by lz on 1/22/17.
//

#ifndef C10K_SERVER_BLOCKING_QUEUE_HPP
#define C10K_SERVER_BLOCKING_QUEUE_HPP


#include <queue>
#include <condition_variable>
#include <mutex>
#include <cstddef>
#include <cstdint>

namespace c10k
{
    namespace detail
    {
        template<typename T>
        class BoundedBlockingQueue
        {
        private:
            std::queue<T> inner;
            const std::size_t upper_bound;

            mutable std::mutex queue_mutex;
            std::condition_variable not_full, not_empty;
        public:
            BoundedBlockingQueue(std::size_t upper_bound):
                upper_bound(upper_bound)
            {}
            BoundedBlockingQueue(const BoundedBlockingQueue &) = delete;
            BoundedBlockingQueue &operator=(const BoundedBlockingQueue &) = delete;

            std::size_t size() const
            {
                std::lock_guard<std::mutex> lk(queue_mutex);
                return inner.size();
            }

            std::size_t is_full() const
            {
                return size() == upper_bound;
            }

            std::size_t is_empty() const
            {
                return size() == 0;
            }

            void push(const T& obj)
            {
                std::unique_lock<std::mutex> lk(queue_mutex);
                if (inner.size() == upper_bound)
                    not_full.wait(lk, [&]() {
                        return inner.size() < upper_bound;
                    });
                inner.push(obj);
                not_empty.notify_one();
            }

            bool try_push(const T& obj)
            {
                std::lock_guard<std::mutex> lk(queue_mutex);
                if (inner.size() == upper_bound)
                    return false;
                inner.push(obj);
                not_empty.notify_one();
                return true;
            }

            void pop(T &obj_into)
            {
                std::unique_lock<std::mutex> lk(queue_mutex);
                if (inner.size() == 0)
                    not_empty.wait(lk, [&]() {
                        return inner.size() > 0;
                    });
                obj_into = inner.front();
                inner.pop();
                not_full.notify_one();
            }

            bool try_pop(T &obj_into)
            {
                std::lock_guard<std::mutex> lk(queue_mutex);
                if (inner.size() == 0)
                    return false;
                obj_into = inner.front();
                inner.pop();
                not_full.notify_one();
                return true;
            }
        };

        // TODO: Don't use BoundedBlockingQueue here
        template<typename T>
        class BlockingQueue: private BoundedBlockingQueue<T>
        {
        public:
            BlockingQueue():
                    BoundedBlockingQueue<T>(SIZE_MAX)
            {}

            BlockingQueue(const BlockingQueue &) = delete;
            BlockingQueue& operator=(const BlockingQueue &) = delete;

            using BoundedBlockingQueue<T>::size;
            using BoundedBlockingQueue<T>::is_empty;
            using BoundedBlockingQueue<T>::is_full;
            using BoundedBlockingQueue<T>::push;
            using BoundedBlockingQueue<T>::pop;
            using BoundedBlockingQueue<T>::try_push;
            using BoundedBlockingQueue<T>::try_pop;
        };
    }
}
#endif //C10K_SERVER_BLOCKING_QUEUE_HPP
