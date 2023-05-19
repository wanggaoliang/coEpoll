#pragma once
#include "NonCopyable.h"
#include "FDICU.h"
#include "FileWQ.h"
#include "../utils/LockFreeQueue.h"
#include <memory>
#include <vector>
#include <queue>
#include <map>
#include <thread>
#include <chrono>
#include <atomic>
class CoKernel;
class Core: NonCopyable
{
public:

    Core(CoKernel *);
    ~Core();

    void loop();

    void quit();

    bool isInLoopThread() const
    {
        return threadId_ == std::this_thread::get_id();
    };

    void assertInLoopThread();

    template <WQCallbackType T>
    void queueInLoop(T &&cb)
    {
        funcs_.enqueue(std::forward<T>(cb));
        if (!isInLoopThread() || !looping_.load(std::memory_order_acquire))
        {
            wakeup();
        }
    }

    template <WQCallbackType T>
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
    void wakeUp();

    std::unique_ptr<FDICU> fdICU_;
    
    std::atomic<bool> looping_;
    std::atomic<bool> quit_;
    int wakeupFd_;
    
    MpscQueue<WQCallback> funcs_;
    CoKernel *kernel_;
    std::thread::id threadId_;
    std::unique_ptr<FileWQ> wakeUpWQ_;

    std::atomic<int> irqNum_;
    const int index;

    static std::atomic<int> core_i;

};
