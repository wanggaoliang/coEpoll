
#pragma once
#include "WQAbstract.h"
#include <queue>
#include <vector>
#include <chrono>

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

    TimeWQ();
    
    TimeWQ(int fd);

    ~TimeWQ();

    void wakeup(uint32_t revent) override;

    template<WQCallbackType T>
    void addWait(T &&cb, const TimePoint &when)
    {
        items_.emplace(std::forward(cb), when);
        resetTimerfd(items_.top().when_);
    }

    template<WakeCallbackType T>
    void setWakeCallback(T &&func)
    {
        wakeCallback = std::forward(func);
    }
private:
    int readTimerfd();

    int resetTimerfd(const TimePoint &expiration);
    std::priority_queue<WaitItem, std::vector<WaitItem>, std::greater<WaitItem>> items_;
    WakeCallback wakeCallback;
};