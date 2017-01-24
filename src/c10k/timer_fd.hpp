//
// Created by lz on 1/21/17.
//

#ifndef C10K_SERVER_TIMER_FD_HPP
#define C10K_SERVER_TIMER_FD_HPP


#include <sys/timerfd.h>
#include <unistd.h>
#include <chrono>

// A fd that becomes readable periodicly
class TimerFD
{
    int fd;
public:
    TimerFD(std::chrono::nanoseconds interval):
            fd(timerfd_create(CLOCK_MONOTONIC, 0))
    {
        itimerspec its {};
        its.it_interval.tv_nsec = interval.count();
        its.it_value = its.it_interval;

        timerfd_settime(fd, 0, &its, nullptr);
    }

    int getfd() const
    {
        return fd;
    }

    ~TimerFD()
    {
        ::close(fd);
    }
};
#endif //C10K_SERVER_TIMER_FD_HPP
