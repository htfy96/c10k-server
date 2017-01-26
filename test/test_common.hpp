//
// Created by lz on 1/24/17.
//

#ifndef C10K_SERVER_TEST_COMMON_HPP
#define C10K_SERVER_TEST_COMMON_HPP


#include <c10k/utils.hpp>
#include <c10k/endian.hpp>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
inline void make_socket_nonblocking(int socketfd)
{
    using namespace c10k::detail;
    int flags = call_must_ok(fcntl, "F_GETFL", socketfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    call_must_ok(fcntl, "F_SETFL", socketfd, F_SETFL, flags);
}

inline int create_socket(bool blocking = true)
{
    using namespace c10k::detail;
    int socketfd = call_must_ok(::socket, "Create socket", AF_INET, SOCK_STREAM, 0);

    static const int ENABLE = 1;
    if (!blocking) {
        make_socket_nonblocking(socketfd);
    }
    call_must_ok(setsockopt, "setsockopt(REUSEADDR)", socketfd, SOL_SOCKET, SO_REUSEADDR, &ENABLE, sizeof(ENABLE));

    return socketfd;
}

inline sockaddr_in create_addr(const char *in_addr, int port)
{
    using namespace c10k::detail;
    using namespace c10k;
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    call_must_ok(inet_aton, "inet_aton", in_addr, &addr.sin_addr);
    addr.sin_port = to_net16(port);
    return addr;
}

inline void cur_sleep_for(std::chrono::milliseconds ms)
{
    std::this_thread::sleep_for(ms);
}

#endif //C10K_SERVER_TEST_COMMON_HPP
