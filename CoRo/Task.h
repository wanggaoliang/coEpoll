#include "Lazy.h"
#include "../CoKernel/CoKernel.h"

class TaskPromise
{
public:
    struct InitAwaiter
    {
        bool await_ready() const noexcept { return false; }

        template <typename PromiseType>
        auto await_suspend(std::coroutine_handle<PromiseType> h) noexcept
        {
            CoKernel::getKernel()->schedule([h = this->_handle]() mutable { h.resume(); });
        }

        void await_resume() noexcept {}
    };
public:
    TaskPromise() : _index(cnt++) {}

    InitAwaiter initial_suspend() noexcept { return {}; }

    std::suspend_never final_suspend() noexcept { return {}; }

    template <typename Awaitable>
        requires HasCoAwaitMethod<Awaitable>
    auto await_transform(Awaitable &&awaitable)
    {
        return std::forward<Awaitable>(awaitable).coAwait();
    }

    std::coroutine_handle<> _continuation;
    uint _index;
    static uint cnt;
};