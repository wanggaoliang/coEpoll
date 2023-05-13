#pragma once
#include "TimeWQ.h"
#include "FileWQ.h"
#include "ThreadCore.h"
#include "../utils/SpinLock.h"
#include <mutex>
#include <memory>
#include <map>
#include <coroutine>
#include <type_traits>
#include <concepts>

template<typename Res>
struct RunInCore
{
    using ret_type = Res;

    template <typename F>
    RunInCore(Core *core, F &&func) : func_(std::forward<F>(func)), core_(core)
    {
    }

    bool await_ready() const noexcept
    {
        if (core_->isInLoopThread())
        {
            ret = func_();
            return true;
        }
        else
        {
            return false;
        }
    }

    void await_suspend(std::coroutine_handle<> handle)
    {
        core_->queueInLoop([h = std::move(handle), func = std::move(func_), &ret]() mutable -> void
                           {
            ret = func();
            h.resume(); });
    }

    ret_type await_resume() noexcept
    {
        return ret;
    }

private:
    Core *core_;
    ret_type ret;
    std::function<ret_type()> func_;
};

template <>
struct RunInCore<void>
{
    using ret_type = void;

    template <typename F>
    RunInCore(Core *core, F &&func) : func_(std::forward<F>(func)), core_(core)
    {
    }

     bool await_ready() const noexcept
    {
        if (core_->isInLoopThread())
        {
            func_();
            return true;
        }
        else
        {
            return false;
        }
    }

    void await_suspend(std::coroutine_handle<> handle)
    {
        core_->queueInLoop([h = std::move(handle), func = std::move(func_)]() mutable -> void
                           {
            func();
            h.resume(); });
    }

    void await_resume() noexcept
    {
    }

private:
    Core *core_;
    std::function <void()> func_;
};

template <typename F>
    requires std::invocable<F>
RunInCore(F &&func) -> RunInCore<decltype(std::declval<F>()())>;


class CoKernel
{
public:
    using FileWQMap = std::map<int, std::shared_ptr<FileWQ>>;
    using ThreadCorePtr = std::shared_ptr<ThreadCore>;
    using ReqIRQRet = RunInCore<int>;
    template <WQCallbackType T>
    void waitFile(T &&cb, int fd, uint32_t events)
    {
        auto fileWQ = fileWQPtrs_.find(fd);

        if (fileWQ == fileWQs_.end())
        {
            return;
        }

        fileWQ->second->addWait(std::forward(cb), events);
    }

    template <WQCallbackType T>
    void waitTime(T &&cb, const TimeWQ::TimePoint &tp)
    {
        if (!timerWQPtr_)
        {
            timerWQPtr_.reset(new TimeWQ());
        }
        timerWQPtr_->addWait(std::forward(cb), tp);
    }

    void waitLock();

    void unlock();

    int createMutex();

    int delMutext();

    ReqIRQRet updateFileWQ(int, uint32_t);

    void removeFileWQ(int);

    void schedule(WQCallback &&);

    void wakeUpReady();

    Core *getBPCore();

private:

    Core *getNextCore();

    /* 只有主线程使用 */
    std::vector<ThreadCorePtr> thCores_;
    std::shared_ptr<Core> core_;

    /* 多线程使用，保证线程安全 */
    std::atomic<uint> nextCore_;
    std::vector<Core *> cores_;
    const uint coreNum_;
    std::unique_ptr<TimeWQ> timerWQPtr_;

    FileWQMap fileWQPtrs_;
    SpinLock fqlk;

    std::queue<WQCallback> readyWQ_;
    std::once_flag once_;
};