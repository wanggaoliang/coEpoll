#pragma once
#include "Common.h"
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

    uint setREvents(uint revents)
    {
        return revents_.exchange(revents, std::memory_order_acq_rel);
    }
    
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

    uint32_t setWEvents(uint wevents)
    {
        return wevents_.exchange(wevents, std::memory_order_acq_rel);
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
    /* 读写多线程 */
    std::atomic<uint> revents_;
    std::atomic<uint> wevents_;

    /* 不可修改 */
    const void *core_;
    int fd_;
};

using WQList = std::vector<WQAbstract>;
using WQPtrList = std::vector<WQAbstract *>;
