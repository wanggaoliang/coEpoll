#include "../CoKernel/CoKernel.h"

class Task;

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
    TaskPromise() : _tcb(cnt++,nullptr) {}

    InitAwaiter initial_suspend() noexcept { return {}; }

    std::suspend_never final_suspend() noexcept { return {}; }

    Task get_return_object() noexcept;

    template <typename Awaitable>
    auto await_transform(Awaitable &&awaitable)
    {
        if constexpr (HasCoAwaitMethod<Awaitable>)
        {
            return std::forward<Awaitable>(awaitable).coAwait();
        }
        else
        {
            return std::forward<Awaitable>(awaitable);
        }
    }

    UTCB _tcb;
    static int cnt;
};

int TaskPromise::cnt = 1;

class Task
{
public:
    using promise_type = TaskPromise;
    Task(std::coroutine_handle<promise_type> coro) :h_(coro) {}
    ~Task() = default;
private:
    std::coroutine_handle<promise_type> h_;
};

