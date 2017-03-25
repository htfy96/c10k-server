//
// Created by lz on 3/1/17.
//

#ifndef C10K_SERVER_ADDR_HPP
#define C10K_SERVER_ADDR_HPP
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>

namespace c10k
{
    // SocketAddress represents an ipv4 address
    // TODO: Add more options
    class SocketAddress
    {
    public:
        SocketAddress(const char *ipaddr, int port);
        const char *ip() const;
        int port() const;
        void setip(const char *new_ip);
        void setport(int new_port);
        sockaddr_in& mutable_addrin();
        const sockaddr_in& addrin() const;
    private:
        sockaddr_in addrin_;
    };
}

#endif //C10K_SERVER_ADDR_HPP
