#pragma once
#include "WQAbstract.h"
#include <functional>
#include <list>
#include <unistd.h>

class FileWQ:public WQAbstract
{
public:
    struct waitItem
    {
        WQCallback func_;
        uint32_t events_;
        waitItem(const WQCallback &func, uint32_t events)
            :func_(func), events_(events)
        {}
        waitItem(WQCallback &&func, uint32_t events)
            :func_(std::move(func)), events_(events)
        {}
    };

    FileWQ(int fd, void* core):WQAbstract(fd,core)
    {

    }

    ~FileWQ() = default;
    void wakeup() override
    {
        for (auto iter = items.begin(); iter != items.end(); )
        {
            if (!revents_)
            {
                break;
            }
            else if (iter->events_ & revents_)
            {
                if (wakeCallback)
                {
                    wakeCallback(std::move(iter->func_));
                }
                iter = items.erase(iter);
                revents_ &= ~(iter->events_ & revents_);
            }
            else
            {
                iter++;
            }
        }
    }

    template<WQCallbackType T>
    void addWait(T &&func, uint32_t events)
    {
        items.emplace_back(std::forward(func), events);
    }

    template<WakeCallbackType T>
    void setWakeCallback(T &&func)
    {
        wakeCallback = std::forward(func);
    }
private:
    std::list<waitItem> items;
    WakeCallback wakeCallback;
};
