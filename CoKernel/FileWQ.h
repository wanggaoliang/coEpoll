#pragma once
#include "Common.h"
#include "WQAbstract.h"
#include <list>
using WQCB = std::function<void(int, uint)>;
using FDCB = std::function<void(int)>;
class FileWQ :public WQAbstract
{
public:
    struct waitItem
    {
        WQCB cb_;
        std::coroutine_handle<> h_;
        uint32_t events_;
        waitItem(const std::coroutine_handle<> &h, uint32_t events, WQCB&& cb)
            :h_(h), events_(events),cb_(std::move(cb))
        {}
    };

    FileWQ(int fd, void *core);

    ~FileWQ();
    
    void wakeup() override;
    
    template<typename T>
    requires std::is_convertible_v<T, WQCB>
    void addWait(const std::coroutine_handle<> &h, uint32_t events, T &&cb)
    {
        items_.emplace_back(h, events,std::forward<T>(cb));
    }

    template<typename T>
    requires std::is_convertible_v<T,WakeCallback>
    void setWakeCallback(T &&func)
    {
        wakeCallback_ = std::forward<T>(func);
    }

    template<typename T>
    requires std::is_convertible_v<T,FDCB>
    void setFDCallback(T &&func)
    {
        fdCallbck_ = std::forward<T>(func);
    }
private:
    std::list<waitItem> items_;
    WakeCallback wakeCallback_;
    FDCB fdCallbck_;
};

