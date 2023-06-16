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
        uint events_;
        waitItem(const std::coroutine_handle<> &h, uint events, WQCB&& cb)
            :h_(h), events_(events),cb_(std::move(cb))
        {}
    };

    FileWQ(int fd, void *core):WQAbstract(fd), core_(core) {}

    ~FileWQ();
    
    void wakeup() override;

    void *getCore() const
    {
        return core_;
    }
    
    template<typename T>
    requires std::is_convertible_v<T, WQCB>
    void addWait(const std::coroutine_handle<> &h, uint events, T &&cb)
    {
        items_.emplace_back(h, events,std::forward<T>(cb));
    }

    template<typename T>
    requires std::is_convertible_v<T,WakeCB>
    void setWakeCallback(T &&func)
    {
        wakeCB_ = std::forward<T>(func);
    }

    template<typename T>
    requires std::is_convertible_v<T,FDCB>
    void setFDCallback(T &&func)
    {
        fdCallbck_ = std::forward<T>(func);
    }
private:
    std::list<waitItem> items_;
    WakeCB wakeCB_;
    FDCB fdCallbck_;
    void *const core_;
};

