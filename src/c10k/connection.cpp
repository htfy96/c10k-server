//
// Created by lz on 1/24/17.
//

#include "connection.hpp"
#include <functional>
#include <system_error>
#include <stdexcept>

//TODO: monitor POLLIN/POLLOUT individually
namespace c10k
{
    void Connection::register_event()
    {
        using namespace std::placeholders;
        if (!registered.exchange(true)) {
            logger->trace("Event registered");
            el.add_event(fd, EventType {EventCategory::POLLRDHUP},
                         std::bind(&Connection::event_handler, this, _1));
            event_listen.set(EventCategory::POLLRDHUP);
        }
    }

    void Connection::remove_event()
    {
        if (registered.exchange(false)) {
            logger->trace("Event removed");
            el.remove_event(fd);
        }
    }

    void Connection::enable_event(EventCategory ec)
    {
        using namespace std::placeholders;
        std::lock_guard<std::recursive_mutex> lk(mutex);
        if (!event_listen.is(ec) && !is_closed())
        {
            event_listen.set(ec);
            logger->trace("Enable event listen: cur={}", event_listen);
            el.modify_event(fd, event_listen, std::bind(&Connection::event_handler, this, _1));
        }
    }

    void Connection::disable_event(EventCategory ec)
    {
        using namespace std::placeholders;
        std::lock_guard<std::recursive_mutex> lk(mutex);
        if (event_listen.is(ec) && !is_closed())
        {
            event_listen.unset(ec);
            logger->trace("Disable event listen: cur={}", event_listen);
            el.modify_event(fd, event_listen, std::bind(&Connection::event_handler, this, _1));
        }
    }

    void Connection::handle_read()
    {
        logger->trace("Handling read, there are {} items in r_buffer", r_buffer.size());
        while (!r_buffer.empty()) // can read
        {
            detail::ConnRReq &req = r_buffer.front();
            logger->trace("Read the first request: {} / {}", req.buf.size(), req.requested_len);
            for (;req.buf.size() < req.requested_len;)
            {
                int read_len = std::min(1024, req.requested_len - (int)req.buf.size());
                req.buf.resize(req.buf.size() + read_len);
                int read_result = ::read(fd, &*req.buf.end() - read_len, read_len);
                logger->trace("Performing read {} bytes, result={}", read_len, read_result);
                if (read_result < 0)
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                        throw std::system_error(read_result, std::system_category());
                    else
                        break;
            }

            if (req.buf.size() == req.requested_len) // read OK
            {
                logger->trace("Req read OK, executing callback");
                req.exec_callback(&*req.buf.begin(), &*req.buf.end());
                r_buffer.pop();
            } else // wait for next time
            {
                break;
            }
        }
        if (r_buffer.empty())
            disable_event(EventCategory::POLLIN);
    }

    void Connection::handle_write()
    {
        logger->trace("Handling write, there are {} items in w_buffer", w_buffer.size());
        while (!w_buffer.empty()) // can write
        {
            detail::ConnWReq &req = w_buffer.front();
            logger->trace("Write the first request: {} / {}", req.offset, req.buf.size());
            int write_result;
            for(;req.offset < req.buf.size();)
            {
                int write_len = std::min(1024, (int)req.buf.size() - req.offset);
                logger->trace("Performing write {} bytes from off={}", write_len, req.offset);
                int write_result = ::write(fd, &*req.buf.begin() + req.offset, write_len);
                logger->trace("Write result={}", write_result);
                if (write_result < 0)
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                        throw std::system_error(write_result, std::system_category());
                    else
                        break;
                req.offset += write_result;
            }
            if (req.offset == req.buf.size()) // write OK
            {
                logger->trace("Req write OK, executing callback");
                req.exec_callback();
                w_buffer.pop();
            } else
            {
                break; // wait for next time
            }
        }
        if (w_buffer.empty())
            disable_event(EventCategory::POLLOUT);
    }

    void Connection::event_handler(const Event &e)
    {
        logger->debug("Handling event {}", e);
        if (e.event_type.is_err())
            close();
        else
        {
            std::lock_guard<std::recursive_mutex> lk(mutex);
            try {
                if (e.event_type.is(EventCategory::POLLIN))
                    handle_read();
                if (e.event_type.is(EventCategory::POLLOUT))
                    handle_write();
            } catch(std::exception &e)
            {
                logger->warn("Socket closed when processing event, closing connection:", e.what());
                close();
            }
        }
    }

}