
#pragma once
#include "Common.h"
#include "WQAbstract.h"
#include <chrono>
#include <list>

using TimePoint = std::chrono::steady_clock::time_point;
using TimeInterval = std::chrono::microseconds;

class TimeWQ :public WQAbstract
{
public:

    using TimerId = uint64_t;
    struct WaitItem
    {
        std::coroutine_handle<> h_;
        TimePoint when_;

        WaitItem(const std::coroutine_handle<> &h,
                 const TimePoint &when) :
            h_(h),
            when_(when)
        {}
    };

    TimeWQ();

    TimeWQ(int fd) :WQAbstract(fd) {}

    ~TimeWQ();

    void wakeup() override;

    void addWait(const std::coroutine_handle<> &, const TimePoint &);

    template<typename T>
    requires std::is_convertible_v<T,WakeCB>
    void setWakeCallback(T &&func)
    {
        wakeCB_ = std::forward<T>(func);
    }
private:
    int readTimerfd();

    int resetTimerfd(const TimePoint &expiration);
    std::list<WaitItem> items_;
    WakeCB wakeCB_;
};