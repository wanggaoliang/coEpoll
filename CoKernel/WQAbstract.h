#pragma once
#include <type_traits>
#include <functional>
#include <vector>
#include <atomic>
#include <unistd.h>
class WQAbstract
{
public:
    WQAbstract(int fd,void* core):revents_(0), wevents_(0),fd_(fd),core_(core){}
    ~WQAbstract()
    {
        if (fd_ >= 0)
        {
            ::close(fd_);
            fd_ = -1;
        }
    }
    virtual void wakeup() = 0;
    
    uint addREvents(uint revents)
    {
        return revents_.fetch_or(revents, std::memory_order_acq_rel);
    }
    uint removeREvents(uint revents)
    {
        return revents_.fetch_and(~revents, std::memory_order_acq_rel);
    }

    uint32_t addWEvents(uint wevents)
    {
        return wevents_.fetch_or(wevents, std::memory_order_acq_rel);
    }

    uint32_t removeWEvents(uint wevents)
    {
        return wevents_.fetch_and(~wevents, std::memory_order_acq_rel);
    }

    int getFd() const
    {
        return fd_;
    }

    void* getArg() const
    {
        return core_;
    }

protected:
    std::atomic<uint> revents_;
    std::atomic<uint> wevents_;

    void *core_;
    int fd_;
};

using WQCallback = std::function<void()>;
using WakeCallback = std::function<void(WQCallback &&)>;
using FDCallback = std::function<void(int)>;
using WQList = std::vector<WQAbstract>;
using WQPtrList = std::vector<WQAbstract *>;

template<typename T>
concept WQCallbackType = std::is_same_v<T, WQCallback>;

template<typename T>
concept WakeCallbackType = std::is_same_v<T, WakeCallback>;

template<typename T>
concept FDCallbackType = std::is_same_v<T, FDCallback>;