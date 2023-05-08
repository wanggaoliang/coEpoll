#include "TimeWQ.h"
#include <chrono>
#include <sys/timerfd.h>
#include <memory.h>



TimeWQ::TimeWQ(void *core):WQAbstract(-1, core)
{
    fd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
}

TimeWQ::TimeWQ(int fd,void* core):WQAbstract(fd,core)
{}

TimeWQ::~TimeWQ()
{
}

void TimeWQ::wakeup()
{
    const auto now = std::chrono::steady_clock::now();
    readTimerfd();
    while (!items_.empty())
    {
        if (items_.top().when_ <= now)
        {
            if (wakeCallback)
            {
                wakeCallback(std::move(items_.top().func_));
            }
            items_.pop();
        }
        else
        {
            resetTimerfd(items_.top().when_);
            break;
        }

    }
}

int TimeWQ::readTimerfd()
{
    uint64_t howmany;
    ssize_t n = ::read(fd_, &howmany, sizeof howmany);
    if (n != sizeof howmany)
    {
        howmany = -1;
    }
    return howmany;
}

int TimeWQ::resetTimerfd(const TimePoint &expiration)
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
