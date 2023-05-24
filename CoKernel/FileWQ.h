#pragma once
#include "Common.h"
#include "WQAbstract.h"
#include <list>

class FileWQ :public WQAbstract
{
public:
    struct waitItem
    {
        std::coroutine_handle<> h_;
        uint32_t events_;
        waitItem(const std::coroutine_handle<> &h, uint32_t events)
            :h_(h), events_(events)
        {}
    };

    FileWQ(int fd, void *core);

    ~FileWQ();
    void wakeup() override;

    void addWait(const std::coroutine_handle<> &h, uint32_t events)
    {
        items_.emplace_back(h, events);
    }

    template<typename T>
    requires std::is_convertible_v<T,WakeCallback>
    void setWakeCallback(T &&func)
    {
        wakeCallback_ = std::forward<T>(func);
    }

    template<typename T>
    requires std::is_convertible_v<T,FDCallback>
    void setFDCallback(T &&func)
    {
        fdCallback_ = std::forward<T>(func);
    }
private:
    std::list<waitItem> items_;
    WakeCallback wakeCallback_;
    FDCallback fdCallbck_;
};

