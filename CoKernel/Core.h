#pragma once
#include "NonCopyable.h"
#include "EPoller.h"
#include "TimeWQ.h"
#include "FileWQ.h"
#include <memory>
#include <vector>
#include <queue>
#include <map>
#include <chrono>
#include <limits>

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
class Core : NonCopyable
{
public:
    using FileWQMap = std::map<int, std::shared_ptr<FileWQ>>;
    
    Core();
    ~Core();

    void run();
    
    void updateFileWQ(int, uint32_t);

    void removeFileWQ(int);

    template<WQCallbackType T>
    void waitFile(T &&cb, int fd, uint32_t events)
    {
        auto fileWQ = fileWQPtrs_.find(fd);

        if (fileWQ == fileWQs_.end())
        {
            return;
        }

        fileWQ->second->addWait(std::forward(cb), events);
    }

    template<WQCallbackType T>
    void waitTime(T &&cb, const TimeWQ::TimePoint &tp)
    {
        if (!timerWQPtr_)
        {
            timerWQPtr_.reset(new TimeWQ());
        }
        timerWQPtr_->addWait(std::forward(cb), tp);
    }

    void schedule(WQCallback &&);

    int createSock();
    void closeSock(int);
private:
    std::unique_ptr<EPoller> poller_;
    std::unique_ptr<TimeWQ> timerWQPtr_;
    FileWQMap fileWQPtrs_;
    std::queue<WQCallback> readyWQ_;
};
