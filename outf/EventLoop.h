#pragma once
#include "NonCopyable.h"
#include "EPoller.h"
#include "TimeWQ.h"
#include "FileWQ.h"
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <map>
#include <chrono>
#include <limits>
#include <atomic>

enum
{
    InvalidTimerId = 0
};

/**
 * @brief As the name implies, this class represents an event loop that runs in
 * a perticular thread. The event loop can handle network I/O events and timers
 * in asynchronous mode.
 * @note An event loop object always belongs to a separate thread, and there is
 * one event loop object at most in a thread. We can call an event loop object
 * the event loop of the thread it belongs to, or call that thread the thread of
 * the event loop.
 */
class EventLoop : NonCopyable
{
public:
    using FileWQMap = std::map<int, std::shared_ptr<FileWQ>>;
    
    EventLoop();
    ~EventLoop();

    



    /**
     * @brief Return true if the event loop is running.
     *
     * @return true
     * @return false
     */
    bool isRunning()
    {
        return looping_.load(std::memory_order_acquire) &&
               (!quit_.load(std::memory_order_acquire));
    }

    void run();
    
    void updateFileWQ(int, uint32_t);

    void removeFileWQ(int);

    template<WQCallbackType T>
    void schedule(T &&, int, uint32_t)
    {
        auto fileWQ = fileWQPtrs_.find(fd);

        if (fileWQ == fileWQs_.end())
        {
            return;
        }

        fileWQ->second->addWait(std::forward(cb), events);
    }

    template<WQCallbackType T>
    void schedule(T &&, const TimeWQ::TimePoint &)
    {
        if (!timerWQPtr)
        {
            timerWQPtr.reset(new TimeWQ());
        }
        timerWQPtr->addWait(std::forward(cb), tp);
    }

private:
    std::atomic<bool> looping_;
    std::atomic<bool> quit_;
    
    std::unique_ptr<EPoller> poller_;
    std::unique_ptr<TimeWQ> timerWQPtr_;
    FileWQMap fileWQPtrs_;
    std::queue<WQCallback> readyWQ_;
};
