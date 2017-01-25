//
// Created by lz on 1/21/17.
//

#ifndef C10K_SERVER_EVENT_LOOP_HPP
#define C10K_SERVER_EVENT_LOOP_HPP


#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory>
#include <cstdlib>
#include <unordered_map>
#include <functional>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <mutex>
#include <initializer_list>
#include "utils.hpp"

namespace c10k
{

    enum struct EventCategory
    {
        POLLIN = EPOLLIN,
        POLLOUT = EPOLLOUT,
        POLLRDHUP = EPOLLRDHUP,
        POLLERR = EPOLLERR,
        POLLHUP = EPOLLHUP
    };

    struct EventType
    {
    private:
        int event_type_;
    public:
        explicit EventType(int et):
                event_type_(et)
        {}


        explicit EventType(EventCategory ec):
                EventType((int)ec)
        {}

        EventType(std::initializer_list<EventCategory> init_list):
                event_type_(0)
        {
            for (auto ec: init_list)
                set(ec);
        }

        EventType():
            event_type_(0)
        {}

        bool is(EventCategory ec) const
        {
            return static_cast<bool>(event_type_ & (int)ec);
        }

        bool is_err() const
        {
            return is(EventCategory::POLLRDHUP) || is(EventCategory::POLLERR) || is(EventCategory::POLLHUP);
        }

        EventType& set(EventCategory ec)
        {
            event_type_ |= (int) ec;
            return *this;
        }

        EventType &unset(EventCategory ec)
        {
            event_type_ &= ~ (int)ec;
            return *this;
        }

        explicit operator int() const
        {
            return event_type_;
        }

        template<typename OST>
                friend OST &operator <<(OST &o, EventType et)
        {
#define GEN_PRINT_EC_DESC(X_) if (et.is(EventCategory::X_)) o << " " << #X_ << " "
            o << "[";
            GEN_PRINT_EC_DESC(POLLIN);
            GEN_PRINT_EC_DESC(POLLOUT);
            GEN_PRINT_EC_DESC(POLLRDHUP);
            GEN_PRINT_EC_DESC(POLLERR);
            GEN_PRINT_EC_DESC(POLLHUP);
            o << "]";
#undef GEN_PRINT_EC_DESC
            return o;
        }
    };

    class EventLoop;
    struct Event
    {
        EventLoop *event_loop;
        int fd;
        EventType event_type;

        template<typename OST>
                friend OST &operator <<(OST &o, const Event &e)
        {
            o << "Eventloop=" << (void*)e.event_loop << ", " <<
                   "fd=" << e.fd << ", " <<
                                       "event_type=" << e.event_type;
            return o;
        }
    };

    using EventHandler = std::function<void(const Event &)>;
    inline void NullEventHandler(const Event &) {}

    namespace detail
    {
        struct PollData
        {
            int fd;
            EventHandler handler;

            PollData(int fd, EventHandler eh):
                    fd(fd), handler(std::move(eh))
            {}
        };
    }

    // thread-safe Eventloop run in each thread
    class EventLoop
    {
    private:
        int epollfd;
        bool loop_enabled_ = true;
        bool in_loop_ = false;
        std::shared_ptr<spdlog::logger> logger;

        std::unordered_map<int, std::unique_ptr<detail::PollData>> fd_to_poll_data;
        mutable std::mutex map_mutex;
        using LoggerT = decltype(logger);

        // workaround before inline var :)
        static constexpr int epoll_event_buf_size()
        {
            return 1024;
        }

        void handle_events(epoll_event *st, epoll_event *ed);

    public:
        EventLoop(int max_event, const LoggerT &logger);
        EventLoop(const EventLoop &) = delete;
        EventLoop &operator=(const EventLoop &) = delete;

        void loop();

        void add_event(int fd, EventType et, EventHandler handler);

        void remove_event(int fd);

        void modify_event(int fd, EventType et, EventHandler handler);

        // whether in loop
        bool in_loop() const
        {
            return in_loop_;
        }

        // whether loop is enabled
        bool loop_enabled() const
        {
            return loop_enabled_;
        }

        void enable_loop()
        {
            loop_enabled_ = true;
        }

        void disable_loop()
        {
            loop_enabled_ = false;
        }

        std::size_t fd_num() const
        {
            std::lock_guard<std::mutex> lk(map_mutex);
            return fd_to_poll_data.size();
        }

        ~EventLoop();

    };
}
#endif //C10K_SERVER_EVENT_LOOP_HPP
