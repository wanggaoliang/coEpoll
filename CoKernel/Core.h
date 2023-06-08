#pragma once
#include "NonCopyable.h"
#include "FDICU.h"
#include "FileWQ.h"
#include "TimeWQ.h"
#include "../utils/MpmcQueue.h"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
class Core : NonCopyable
{
public:

    Core();
    ~Core();

    void loop();

    void quit();

    void wakeUp();

    bool isInLoopThread() const
    {
        return threadId_ == std::this_thread::get_id();
    };

    void assertInLoopThread();

    template <typename T>
        requires std::is_convertible_v<T, WQCallback>
    void queueInLoop(T &&cb)
    {
        funcs_.enqueue(std::forward<T>(cb));
        if (!isInLoopThread() || !looping_.load(std::memory_order_acquire))
        {
            wakeup();
        }
    }

    template <typename T>
        requires std::is_convertible_v<T, WQCallback>
    void runInLoop(T &&cb)
    {
        if (isInLoopThread())
        {
            cb();
        }
        else
        {
            queueInLoop(std::forward<T>(cb));
        }
    }

    int addIRQ(WQAbstract *WQA)
    {
        return fdICU_->updateIRQ(EPOLL_CTL_ADD, WQA);
    }

    int modIRQ(WQAbstract *WQA)
    {
        return fdICU_->updateIRQ(EPOLL_CTL_MOD, WQA);
    }

    int delIRQ(WQAbstract *WQA)
    {
        return fdICU_->updateIRQ(EPOLL_CTL_DEL, WQA);
    }

    void waitTime(const std::coroutine_handle<> &h, const TimePoint &when)
    {
        timerWQPtr_->addWait(h, when);
    }

    static Core *getCurCore()
    {
        return curCore;
    }

    static void setCurCore(Core *core)
    {
        curCore = core;
    }

public:
    uint irqn;
    uint pos;

private:
    std::unique_ptr<FDICU> fdICU_;
    MpmcQueue<WQCallback> funcs_;

    std::atomic<bool> looping_;
    std::atomic<bool> quit_;

    std::unique_ptr<FileWQ> wakeUpWQ_;
    std::unique_ptr<TimeWQ> timerWQPtr_;

    std::thread::id threadId_;
    static thread_local Core *curCore;
};

thread_local Core* Core::curCore = nullptr;