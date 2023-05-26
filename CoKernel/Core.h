#pragma once
#include "NonCopyable.h"
#include "FDICU.h"
#include "FileWQ.h"
#include "TimeWQ.h"
#include "../utils/LockFreeQueue.h"
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

    int removeIRQ(WQAbstract *WQA)
    {
        return fdICU_->updateIRQ(EPOLL_CTL_DEL, WQA);
    }

private:
    std::unique_ptr<FDICU> fdICU_;
    MpscQueue<WQCallback> funcs_;

    std::atomic<bool> looping_;
    std::atomic<bool> quit_;

    std::unique_ptr<FileWQ> wakeUpWQ_;
    std::unique_ptr<TimeWQ> timerWQPtr_;

    std::queue<std::coroutine_handle<>> readyRo_;

    std::thread::id threadId_;
    std::atomic<uint> irqNum_;
};
