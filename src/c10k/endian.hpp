//
// Created by lz on 1/21/17.
//

#ifndef C10K_SERVER_ENDIAN_HPP
#define C10K_SERVER_ENDIAN_HPP

#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdint>

namespace c10k {
    namespace detail
    {
        inline std::uint32_t to_net32(std::uint32_t host_n)
        {
            return ::htonl(host_n);
        }
        inline std::uint16_t to_net16(std::uint16_t host_n)
        {
            return ::htons(host_n);
        }
        inline std::uint32_t to_host32(std::uint32_t net_n)
        {
            return ::ntohl(net_n);
        }
        inline std::uint16_t to_host16(std::uint16_t net_n)
        {
            return ::ntohs(net_n);
        }
    }
}

#endif //C10K_SERVER_ENDIAN_HPP
