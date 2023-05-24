#include "Common.h"
#include "../CoRo/Lazy.h"
#include "MuCore.h"

using LockHandle = std::coroutine_handle<LazyPromise<void>>;

struct LockAwaiter
{
    MuCore *mu_;

    LockAwaiter(MuCore *mu) :mu_(mu)
    {

    }

    ~LockAwaiter() = default;

    bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(LockHandle h)
    {
        auto old = mu_->waiter_++;
        if (!old)
        {
            mu_->tid_.store(h.promise()._tid);
            return false;
        }
        else
        {
            mu_->items_.enqueue(MuCore::WaitItem{ h.promise()._tid,h });
            return true;
        }
    }

    void await_resume() noexcept
    {}
};

struct UnlockAwaiter
{
    MuCore *mu_;
    WakeCallback wakeCallback_;
    UnlockAwaiter(MuCore *mu, const WakeCallback &cb) :mu_(mu), wakeCallback_(cb)
    {

    };

    UnlockAwaiter(MuCore *mu, WakeCallback &&cb) :mu_(mu), wakeCallback_(std::move(cb))
    {

    };
    
    ~UnlockAwaiter() = default;

    bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(LockHandle h)
    {
        int ul = 0;
        MuCore::WaitItem newItem;
        auto ret = mu_->tid_.compare_exchange_strong(h.promise()._tid, 0);
        if (ret)
        {
            auto old = mu_->waiter_--;
            if (old > 1)
            {
                while (mu_->items_.dequeue(newItem))
                {
                    mu_->tid_.store(newItem.tid_);
                    if (wakeCallback_)
                    {
                        wakeCallback_(std::move(newItem.h_));
                    }
                }
            }
        }
        else
        {
            //throw exception
        }
        return false;
    }

    void await_resume() noexcept
    {}
};
