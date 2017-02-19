//
// Created by lz on 2/19/17.
//

#ifndef C10K_SERVER_PINGPONG_HANDLER_HPP
#define C10K_SERVER_PINGPONG_HANDLER_HPP

#include <c10k/handler.hpp>
#include <c10k/connection.hpp>
#include <vector>
#include <atomic>
#include <sstream>

std::atomic_bool check_ok {true};
std::stringstream ss;

struct ServerHandler: public c10k::Handler
{
    union {
        std::uint16_t len_int;
        char len_chars[2];
    } len;
    std::vector<char> buffer;
    virtual void handle_init(const c10k::ConnectionPtr &conn) override
    {
        using namespace std::placeholders;
        using std::static_pointer_cast;
        C10K_READ_ASYNC_THEN_MEMFUN(conn, len.len_chars, 2, read_buffer);
    }

    void read_buffer(const c10k::ConnectionPtr &conn, char *st, char *ed)
    {
        using namespace std::placeholders;
        buffer.reserve(len.len_int);
        C10K_READ_ASYNC_THEN_MEMFUN(conn, std::back_inserter(buffer), len.len_int, write_back);
    }

    void write_back(const c10k::ConnectionPtr &conn, char *st, char *ed) {
        using namespace std::placeholders;
        std::vector<char> wrt_buffer;
        wrt_buffer.reserve(2 + len.len_int);
        std::copy_n(len.len_chars, 2, std::back_inserter(wrt_buffer));
        std::copy(buffer.begin(), buffer.end(), std::back_inserter(wrt_buffer));
        C10K_WRITE_ASYNC_THEN_MEMFUN(conn, wrt_buffer.begin(), wrt_buffer.end(), close_connection);
    }

    void close_connection(const c10k::ConnectionPtr &conn)
    {
        conn->close();
    }
};

struct ClientHandler: public c10k::Handler
{
    union Len
    {
        std::uint16_t len_int;
        char len_chars[2];
    };
    std::vector<char> gen_data()
    {
        std::vector<char> wrt_buffer;
        Len len;
        len.len_int = std::rand() % 32768;
        wrt_buffer.reserve(2 + len.len_int);
        std::copy_n(len.len_chars, 2, std::back_inserter(wrt_buffer));
        std::generate_n(std::back_inserter(wrt_buffer), len.len_int, []() {
            return std::rand() % 128;;
        });
        return wrt_buffer;
    }

    std::vector<char> send_data, recv_data;
    virtual void handle_init(const c10k::ConnectionPtr &conn) override
    {
        using namespace std::placeholders;
        send_data = gen_data();
        conn->write_async(send_data.cbegin(), send_data.cend());
        conn->read_async_then(std::back_inserter(recv_data), send_data.size(),
                              std::bind(&ClientHandler::check_data,
                                        std::static_pointer_cast<ClientHandler>(shared_from_this()), _1, _2, _3));
    }

    void check_data(const c10k::ConnectionPtr &conn, char *st, char *ed)
    {
        bool ok = true;
        ok &= send_data.size() == recv_data.size();
        if (ok)
            for (int i=0; i<send_data.size(); ++i) {
                ok &= send_data[i] == recv_data[i];
                if (send_data[i] != recv_data[i]) {
                    for (int j=i-5; j < i+5 && j<send_data.size(); ++j)
                        ss << "Pos[" << j << "]: send=" << send_data[j] << " recv=" << recv_data[j] << std::endl;
                    ss << "At pos " << i << " send=" << (int) send_data[i] << " recv=" << (int) recv_data[i] <<
                       " size=" << recv_data.size() <<
                       " fd=" << conn->getFD() << std::endl;
                }
            }
        if (check_ok.load() && !ok) {
            check_ok = ok;
            ss << "Err!" << "Send size="<< send_data.size() <<" recv size=" << recv_data.size() << std::endl;
        }
        conn->close();
    }
};
#endif //C10K_SERVER_PINGPONG_HANDLER_HPP
