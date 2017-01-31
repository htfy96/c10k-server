//
// Created by lz on 1/21/17.
//

#include "event_loop.hpp"
#include "timer_fd.hpp"
#include <chrono>

namespace c10k
{

    EventLoop::EventLoop(int max_event, const LoggerT &logger):
            logger(logger)
    {
        using detail::call_must_ok;
        epollfd = call_must_ok(epoll_create, "Create epoll fd", max_event);
        logger->debug("EPOLLFD created: {}", epollfd);
    }

    void EventLoop::loop()
    {
        using detail::make_scope_guard;
        auto in_loop_guard = make_scope_guard(
                [&]() {
                    in_loop_ = true;
                    logger->debug("Start event loop");
                },
                [&]() {
                    in_loop_ = false;
                    logger->debug("Exit event loop");
                }
        );

        epoll_event events[epoll_event_buf_size()];
        using namespace std::chrono_literals;
        TimerFD fd(25ms);

        auto fd_guard = make_scope_guard(
                [&]() {
                    add_event(fd.getfd(), EventType(EventCategory::POLLIN), EventLoop::NullEventHandler);
                },
                [&]() {
                    remove_event(fd.getfd());
                });

        for (;loop_enabled_;)
        {
            int ret = epoll_wait(epollfd, events, epoll_event_buf_size(), -1);
            if (ret >= 0)
                handle_events(events, events + ret);
            else
            {
                std::error_code ec(errno, std::system_category());
                logger->critical("Error in epoll_wait: {} epollfd={} errno={}", ec.message(), epollfd, errno);
                std::lock_guard<std::mutex> lk(map_mutex);
                for (auto &&pair : fd_to_poll_data)
                {
                    logger->debug("fd {} listening", pair.first);
                }
                throw std::system_error(ec);
            }
        }
    }

    void EventLoop::handle_events(epoll_event *st, epoll_event *ed)
    {
        std::for_each(st, ed, [&](const epoll_event &ev) {
            detail::PollData *data = static_cast<detail::PollData *>(ev.data.ptr);
            Event e;
            EventHandler handler;
            {
                std::lock_guard<std::mutex> lk(map_mutex);
                e = Event{this, data->fd, EventType(ev.events)};
                handler = data->handler;
            }
            logger->trace("Distributed to handle event {}", e);
            handler(e);
        });
    }

    void EventLoop::add_event(int fd, EventType et, EventHandler handler)
    {
        using detail::PollData;
        using detail::call_must_ok;

        std::lock_guard<std::mutex> lk(map_mutex);

        logger->debug("Add event {} to fd={}", et, fd);
        epoll_event new_event;
        new_event.events = (int)et;
        auto it = fd_to_poll_data.insert(std::make_pair(fd, std::make_unique<PollData>(fd, std::move(handler))));
        if (!it.second)
        {
            logger->error("Cannot insert fd {}: already exist",fd);
            throw std::runtime_error("Cannot insert fd into event(already exist)");
        }
        PollData *ptr = it.first->second.get();

        new_event.data.ptr = (void*) ptr;
        call_must_ok(epoll_ctl, "epoll_ctl ADD", epollfd, EPOLL_CTL_ADD, fd, &new_event);
    }

    void EventLoop::remove_event(int fd)
    {
        using detail::call_must_ok;

        std::lock_guard<std::mutex> lk(map_mutex);
        logger->debug("Remove fd={}", fd);
        call_must_ok(epoll_ctl, "epoll_ctl REMOVE", epollfd, EPOLL_CTL_DEL, fd, nullptr);
        auto it = fd_to_poll_data.find(fd);
        if (it == fd_to_poll_data.end())
        {
            logger->error("Cannot remove fd {} from map", fd);
            throw std::runtime_error("Cannot remove fd from map");
        }
        fd_to_poll_data.erase(it);
    }

    void EventLoop::modify_event(int fd, EventType et, EventHandler handler)
    {
        using detail::PollData;
        using detail::call_must_ok;

        std::lock_guard<std::mutex> lk(map_mutex);
        logger->debug("Modify event {} to fd={}", et, fd);
        epoll_event new_event;

        new_event.events = (int)et;
        auto it = fd_to_poll_data.find(fd);

        if (it == fd_to_poll_data.end())
        {
            logger->error("No old event registered with fd={}", fd);
            throw std::runtime_error("No old event registered with this fd when modifying");
        }

        it->second->fd = fd;
        it->second->handler = std::move(handler);
        new_event.data.ptr = (void*)it->second.get();

        call_must_ok(epoll_ctl, "epoll_ctl MODIFY", epollfd, EPOLL_CTL_MOD, fd, &new_event);
    }

    EventLoop::~EventLoop()
    {
        using detail::call_must_ok;
        logger->info("Closing epollfd={}", epollfd);
        call_must_ok(::close, "Close", epollfd);
    }
}