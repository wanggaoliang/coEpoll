
#pragma once
#include "WQAbstract.h"
#include <queue>
#include <vector>
#include <chrono>
#include <sys/timerfd.h>
#include <memory.h>
#include <unistd.h>

class TimeWQ:public WQAbstract
{
public:

    using TimerId = uint64_t;
    using TimePoint = std::chrono::steady_clock::time_point;
    using TimeInterval = std::chrono::microseconds;
    struct WaitItem
    {
        WQCallback func_;
        TimePoint when_;

        WaitItem(const WQCallback &cb,
                 const TimePoint &when):
            func_(cb),
            when_(when)
        {

        }

        WaitItem(WQCallback &&cb,
                 const TimePoint &when):
            func_(std::move(cb)),
            when_(when)
        {

        }
        bool operator<(const WaitItem &t) const { return when_ < t.when_; }
        bool operator>(const WaitItem &t) const { return when_ > t.when_; }
    };

    TimeWQ()
    {
        fd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    }

    TimeWQ(int fd):fd_(fd)
    {}

    ~TimeWQ()
    {
        if (fd_ >= 0)
        {
            ::close(fd_);
            fd_ = -1;
        }
    }

    void wakeup(uint32_t revent) override
    {
        const auto now = std::chrono::steady_clock::now();
        readTimerfd();
        while (!items_.empty())
        {
            if (items_.top().when_ <= now)
            {
                items_.top().func_();
                items_.pop();
            }
            else
            {
                resetTimerfd(items_.top().when_);
                break;
            }

        }
    }

    template<IsWQCallback T>
    void addWait(T &&cb, const TimePoint &when)
    {
        items_.emplace(std::forward(cb), when);
        resetTimerfd(items_.top().when_);
    }
private:
    int readTimerfd()
    {
        uint64_t howmany;
        ssize_t n = ::read(fd_, &howmany, sizeof howmany);
        if (n != sizeof howmany)
        {
            howmany = -1;
        }
        return howmany;
    }

    int resetTimerfd(const TimePoint &expiration)
    {
        struct itimerspec newValue;
        struct itimerspec oldValue;
        auto microSeconds = std::chrono::duration_cast<std::chrono::microseconds>(
            expiration - std::chrono::steady_clock::now()).count();
        memset(&newValue, 0, sizeof(newValue));
        memset(&oldValue, 0, sizeof(oldValue));

        if (microSeconds < 100)
        {
            microSeconds = 100;
        }

        newValue.it_value.tv_sec = static_cast<time_t>(microSeconds / 1000000);
        newValue.it_value.tv_nsec = static_cast<long>((microSeconds % 1000000) * 1000);
        int ret = ::timerfd_settime(fd_, 0, &newValue, &oldValue);
        return ret;
    }
    std::priority_queue<WaitItem, std::vector<WaitItem>, std::greater<WaitItem>> items_;
    int fd_;
};