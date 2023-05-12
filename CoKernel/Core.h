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
        funcs_.enqueue(std::forward(cb));
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
            queueInLoop(std::forward(cb));
        }
    }
    
    void addIRQ(WQAbstract *WQA);
    
    void modIRQ(WQAbstract *);
    
    void removeIRQ(WQAbstract *);

private:
    void wakeUp();

    std::atomic<bool> looping_;
    std::atomic<bool> quit_;
    int wakeupFd_;
    std::unique_ptr<FDICU> fdICU_;
    MpscQueue<WQCallback> funcs_;
    CoKernel *kernel_;
    std::thread::id threadId_;
    std::unique_ptr<FileWQ> wakeUpWQ_;

    std::atomic<int> irqNum_;
    const int index;

    static std::atomic<int> core_i;

};
