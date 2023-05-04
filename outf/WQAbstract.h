#pragma once
#include <type_traits>
#include <functional>
#include <vector>
class WQAbstract
{
public:
    WQAbstract(int fd):revents_(0), wevents_(0),fd_(fd){}
    ~WQAbstract() {}
    virtual void wakeup() = 0;
    void setREvents(uint32_t revents)
    {
        revents_ = revents;
    }
    void setWEvents(uint32_t wevents)
    {
        wevents_ = wevents;
    }
    uint32_t getREvents()const
    {
        return revents_;
    }

    uint32_t getWEvents()const
    {
        return wevents_;
    }
    int getFd() const
    {
        return fd_;
    }
protected:
    uint32_t revents_;
    uint32_t wevents_;
    int fd_;
};

using WQCallback = std::function<void()>;
using WakeCallback = std::function<void(WQCallback&&)>;
using WQList = std::vector<WQAbstract>;
using WQPtrList = std::vector<WQAbstract *>;

template<typename T>
concept WQCallbackType = std::is_same_v<T, WQCallback>;

template<typename T>
concept WakeCallbackType = std::is_same_v<T, WakeCallback>;