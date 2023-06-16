#pragma once
#include <vector>
#include <unistd.h>
class WQAbstract
{
public:
    WQAbstract(int fd):revents_(0), wevents_(0),fd_(fd){}
    ~WQAbstract()
    {
        if (fd_ >= 0)
        {
            ::close(fd_);
            fd_ = -1;
        }
    }
    virtual void wakeup() = 0;

    void setREvents(uint revents)
    {
        revents_=revents;
    }
    

    void setWEvents(uint wevents)
    {
        wevents_ = wevents;
    }

    void addREvents(uint revents)
    {
        revents_ |= revents;
    }


    void addWEvents(uint wevents)
    {
        wevents_ |= wevents;
    }

    uint getREvents() const
    {
        return revents_;
    }


    uint getWEvents() const
    {
        return wevents_;
    }


    int getFd() const
    {
        return fd_;
    }

protected:
    /* 读写多线程 */
    uint revents_;
    uint wevents_;
    int fd_;
};

using WQList = std::vector<WQAbstract>;
using WQPtrList = std::vector<WQAbstract *>;
