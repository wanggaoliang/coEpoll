#pragma once
#include "TimeWQ.h"
#include "FileWQ.h"
#include "ThreadCore.h"
#include "MuCore.h"
#include "../CoRo/Lazy.h"
#include <mutex>
#include <memory>
#include <unordered_map>
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
            ret = func(); });
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

struct TimeAwaiter
{
    TimePoint point;
    Core *core;
    TimeAwaiter(const TimePoint &when, Core *co) :point(when), core(co)
    {};

    ~TimeAwaiter() = default;

    bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(LockHandle h)
    {
        core->waitTime(h, point);
        return true;
    }

    void await_resume() noexcept
    {}
};

class CoKernel
{
public:
    using FileWQMap = std::unordered_map<int, std::shared_ptr<FileWQ>>;
    using ThreadCorePtr = std::shared_ptr<ThreadCore>;
    using ReqIRQRet = RunInCore<int>;

    Lazy<void> waitFile(int, uint32_t);

    Lazy<void> waitTime(const TimePoint &);

    Lazy<void> CoRoLock(MuCore &);

    Lazy<void> CoRoUnlock(MuCore &);

    Lazy<int> updateIRQ(int, uint32_t);

    Lazy<int> removeIRQ(int);

    void schedule(std::coroutine_handle<> &);

    void wakeUpReady();

    void start();

    static void INIT(uint num)
    {
        kernel = std::make_shared<CoKernel>(num);
    }

    static std::shared_ptr<CoKernel> getKernel()
    {
        return kernel;
    }

    ~CoKernel() = default;
private:
    CoKernel(uint);

    /* 只有主线程使用 */
    std::vector<ThreadCorePtr> thCores_;
    std::shared_ptr<Core> core_;

    /* 中断注册相关多线程使用，保证线程安全 */
    
    std::vector<Core*> irqs_;
    const uint coreNum_;
    FileWQMap fileWQPtrs_;
    MuCore fMapLk_;

    std::queue<std::coroutine_handle<>> readyRo_;
    /* 处理全在非协程中 */
    std::once_flag once_;
    static std::shared_ptr<CoKernel> kernel;
};

std::shared_ptr<CoKernel> CoKernel::kernel = nullptr;