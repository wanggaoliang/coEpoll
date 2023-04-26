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
        bool exclusive_;
        waitItem(const WQCallback &func, uint32_t events, bool exclusive)
            :func_(func), events_(events), exclusive_(exclusive)
        {}
        waitItem(WQCallback &&func, uint32_t events, bool exclusive)
            :func_(std::move(func)), events_(events), exclusive_(exclusive)
        {}
    };

    FileWQ(int fd_):fd(fd_)
    {

    }

    ~FileWQ() = default;
    void wakeup(uint32_t revent) override
    {
        bool done = false;
        for (auto iter = items.begin(); iter != items.end(); )
        {
            if (iter->events_ & revent)
            {
                iter->func_();
                done = iter->exclusive_;
                iter = items.erase(iter);
                if (done)
                {
                    break;
                }
            }
            else
            {
                iter++;
            }
        }
    }

    template<IsWQCallback T>
    void addWait(T &&func, uint32_t events, bool exclusive = false)
    {
        items.emplace_back(std::forward(func), events, exclusive);
    }
private:
    std::list<waitItem> items;
    int fd;
};
