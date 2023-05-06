#pragma once
#include "TimeWQ.h"
#include "FileWQ.h"
#include "ThreadCore.h"
#include <mutex>
#include <memory>
#include <map>
class CoKernel
{
public:
    using FileWQMap = std::map<int, std::shared_ptr<FileWQ>>;
    using ThreadCorePtr = std::shared_ptr<ThreadCore>;

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

    void updateFileWQ(int, uint32_t);

    void removeFileWQ(int);

    void schedule(WQCallback &&);

    ThreadCorePtr getCore();


private:
   

    struct CoreCmp
    {
        bool
            operator()(ThreadCorePtr __x, ThreadCorePtr __y) const
        {
            return __x->listenNum > __y->listenNum;
        }
    };
    
    std::priority_queue < ThreadCorePtr, std::vector<ThreadCorePtr>, CoreCmp > thCores_;
    std::unique_ptr<TimeWQ> timerWQPtr_;
    FileWQMap fileWQPtrs_;
    std::queue<WQCallback> readyWQ_;
    std::once_flag once_;
    ThreadCorePtr core_;
};