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

    FileWQ(int fd, void *core);

    ~FileWQ();
    void wakeup() override;

    template<WQCallbackType T>
    void addWait(T &&func, uint32_t events)
    {
        items.emplace_back(std::forward<T>(func), events);
    }

    template<WakeCallbackType T>
    void setWakeCallback(T &&func)
    {
        wakeCallback_ = std::forward<T>(func);
    }

    template<FDCallbackType T>
    void setFDCallback(T &&func)
    {
        fdCallback_ = std::forward<T>(func);
    }
private:
    std::list<waitItem> items;
    WakeCallback wakeCallback_;
    FDCallback fdCallbck_;
};

