//
// Created by lz on 2/4/17.
//

#ifndef C10K_SERVER_EXPIRE_HEAP_HPP
#define C10K_SERVER_EXPIRE_HEAP_HPP

#include <chrono>
#include <vector>
#include <list>
#include <unordered_map>
#include <cassert>
namespace c10k
{
    namespace detail
    {
        template<typename T>
        class ExpireRecord
        {
        private:
            const std::chrono::nanoseconds expiration;
            using TimePointType = std::chrono::steady_clock::time_point;

            struct ExpData
            {
                T val;
                TimePointType last_visit;
            };

            std::list<ExpData> last_visit_list;
            using IteratorT = typename std::list<ExpData>::iterator;
            std::unordered_map<T, IteratorT> map_to_node;
        public:
            ExpireRecord(const std::chrono::nanoseconds &expiration);
            inline void push_element(const T& val);
            inline void push_element(T &&val);
            inline void visit(const T& val);

            inline std::vector<T> get_expired_and_remove();
            template<typename FT>
            inline void for_each_expired_and_remove(FT f);
        };


        template<typename T>
        ExpireRecord<T>::ExpireRecord(const std::chrono::nanoseconds &expiration):
                expiration(expiration)
        {}

        template<typename T>
        void ExpireRecord<T>::push_element(const T &val)
        {
            auto iter = last_visit_list.insert(last_visit_list.end(), ExpData{val, std::chrono::steady_clock::now()});
            auto it = map_to_node.insert(std::make_pair(val, iter));
            assert(it.second);
        }

        template<typename T>
        void ExpireRecord<T>::push_element(T &&val)
        {
            auto iter = last_visit_list.insert(last_visit_list.end(), ExpData{val, std::chrono::steady_clock::now()});
            auto it = map_to_node.insert(std::make_pair(std::move(val), iter));
            assert(it.second);
        }

        template<typename T>
        void ExpireRecord<T>::visit(const T &val)
        {
            auto it = map_to_node.find(val);
            assert(it != map_to_node.end());
            IteratorT list_old_iter = it->second;
            ExpData new_data = *list_old_iter;
            new_data.last_visit = std::chrono::steady_clock::now();
            last_visit_list.erase(list_old_iter);
            auto list_new_iter = last_visit_list.insert(last_visit_list.end(), new_data);
            it->second = list_new_iter;
        }

        template<typename T>
        template<typename FT>
        void ExpireRecord<T>::for_each_expired_and_remove(FT f)
        {
            auto now = std::chrono::steady_clock::now();
            for (auto it = last_visit_list.begin();
                 it != last_visit_list.end() && now - it->last_visit >= expiration;
                 )
            {
                f(it->val);
                auto next_it = std::next(it);
                map_to_node.erase(it->val);
                last_visit_list.erase(it);
                it = next_it;
            }
        }

        template<typename T>
        std::vector<T> ExpireRecord<T>::get_expired_and_remove()
        {
            std::vector<T> result; result.reserve(1/8 * last_visit_list.size());
            for_each_expired_and_remove([&result](const T& val) {
                result.push_back(val);
            });
            return result;
        }
    }

}

#endif //C10K_SERVER_EXPIRE_HEAP_HPP
