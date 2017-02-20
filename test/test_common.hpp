//
// Created by lz on 1/24/17.
//

#ifndef C10K_SERVER_TEST_COMMON_HPP
#define C10K_SERVER_TEST_COMMON_HPP


#include <c10k/utils.hpp>
#include <c10k/endian.hpp>
#include <thread>

inline void cur_sleep_for(std::chrono::milliseconds ms)
{
    std::this_thread::sleep_for(ms);
}

template< class Clock, class Duration >
void cur_sleep_until( const std::chrono::time_point<Clock,Duration>& sleep_time )
{
    std::this_thread::sleep_until(sleep_time);
};

using c10k::detail::create_addr;
using c10k::detail::create_socket;
using c10k::detail::make_socket_nonblocking;

#endif //C10K_SERVER_TEST_COMMON_HPP
