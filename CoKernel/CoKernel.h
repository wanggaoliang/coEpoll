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

class CoKernel
{
public:
    using FileWQMap = std::unordered_map<int, std::shared_ptr<FileWQ>>;
    using ThreadCorePtr = std::shared_ptr<ThreadCore>;

    Lazy<int> waitFile(int, uint32_t, WQCB);

    Lazy<void> waitTime(const TimePoint &);

    Lazy<void> CoRoLock(MuCore &);

    Lazy<void> CoRoUnlock(MuCore &);

    Lazy<int> updateIRQ(int, uint32_t);

    Lazy<int> removeIRQ(int);

    void wakeUpReady(std::coroutine_handle<> &);

    bool schedule();

    void start();

    static void INIT(uint num)
    {
        kernel = std::shared_ptr<CoKernel>(new CoKernel(num));
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

    std::vector<Core *> irqs_;
    const uint coreNum_;
    FileWQMap fileWQPtrs_;
    MuCore fMapLk_;

    MpmcQueue<std::coroutine_handle<>> readyRo_;
    /* 处理全在非协程中 */
    static std::shared_ptr<CoKernel> kernel;
};

template<typename Res>
struct XCoreFAwaiter
{
    using ret_type = Res;

    template <typename F>
    XCoreFAwaiter(Core *core, F &&func) : func_(std::forward<F>(func)), core_(core)
    {}

    bool await_ready() noexcept
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
        core_->queueInLoop([h = std::move(handle), func = std::move(func_), this]() mutable -> void {
            ret = func();
            CoKernel::getKernel()->wakeUpReady(h);
                           });
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
struct XCoreFAwaiter<void>
{
    using ret_type = void;

    template <typename F>
    XCoreFAwaiter(Core *core, F &&func) : func_(std::forward<F>(func)), core_(core)
    {}

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
        core_->queueInLoop([h = std::move(handle), func = std::move(func_)]() mutable -> void {
            func();
            CoKernel::getKernel()->wakeUpReady(h);
                           });
    }

    void await_resume() noexcept
    {}

private:
    Core *core_;
    std::function <void()> func_;
};

template <typename F>
    requires std::invocable<F>
XCoreFAwaiter(Core* core, F &&func)->XCoreFAwaiter<decltype(std::declval<F>()())>;

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

    bool await_suspend(std::coroutine_handle<> h)
    {
        core->waitTime(h, point);
        return true;
    }

    void await_resume() noexcept
    {}
};

struct FileAwaiter
{
    template <typename F>
    requires std::is_convertible_v<F,std::function<ioret(int,uint)>>
    FileAwaiter(std::shared_ptr<FileWQ> &fq,uint events,F &&func) : func_(std::forward<F>(func)),fq_(fq),events_(events),err_(0)
    {}

    ~FileAwaiter() = default;

    bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle)
    {
        auto core = static_cast<Core *>(fq_->getCore());
        auto xFunc = [this](int fd,uint events)mutable->bool {
            auto cons = this->func_(fd, events);
            ret = cons.ret;
            err_ = errno;
            return cons.block;
        };
        if (core->isInLoopThread())
        {
            auto revents = fq_->getREvents();
            auto bl = xFunc(fq_->getFd(), 0);
            fq_->setREvents(revents & ~events_);
            if (bl)
            {
                fq_->addWait(handle, events_, std::move(xFunc));
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            core->queueInLoop([h = std::move(handle), func = std::move(xFunc), this]() mutable -> void {
                auto revents = fq_->getREvents();
                auto bl = func(fq_->getFd(), 0);
                fq_->setREvents(revents & ~ events_);
                if (bl)
                {
                    fq_->addWait(h, events_, std::move(func));
                    fq_->wakeup();
                }
                else
                {
                    CoKernel::getKernel()->wakeUpReady(h);
                }
                });
            return true;
        }
    }

    int await_resume() noexcept
    {
        errno = err_;
        return ret;
    }

private:
    int ret;
    int err_;
    std::shared_ptr<FileWQ> fq_;
    uint events_;
    std::function<ioret(int, uint)> func_;
};
