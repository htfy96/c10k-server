//
// Created by lz on 3/1/17.
//

#include "addr.hpp"
#include "endian.hpp"
#include "utils.hpp"
#include <system_error>

namespace c10k
{
    SocketAddress::SocketAddress(const char *ipaddr, int port)
    {
        addrin_.sin_family = AF_INET;
        setip(ipaddr);
        setport(port);
    }

    const char* SocketAddress::ip() const
    {
        const char *result = ::inet_ntoa(addrin_.sin_addr);
        if (!result)
            throw std::system_error();
        return result;
    }

    int SocketAddress::port() const
    {
        using detail::to_host16;
        return detail::to_host16(addrin_.sin_port);
    }

    void SocketAddress::setip(const char *new_ip)
    {
        using detail::call_must_ok;
        call_must_ok(inet_aton, "inet_aton", new_ip, &addrin_.sin_addr);
    }

    void SocketAddress::setport(int new_port)
    {
        using detail::to_net16;
        addrin_.sin_port = to_net16(new_port);
    }

    const sockaddr_in& SocketAddress::addrin() const
    {
        return addrin_;
    }

    sockaddr_in& SocketAddress::mutable_addrin()
    {
        return addrin_;
    }
}
