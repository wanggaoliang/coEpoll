#pragma once
#include "RoCommon.h"

class Task;

class TaskPromise
{

public:
    TaskPromise() : _tcb(cnt++,nullptr) {}

    std::suspend_never initial_suspend() noexcept { return {}; }

    std::suspend_never final_suspend() noexcept { return {}; }
    
    Task get_return_object() noexcept;
    template <typename Awaitable>
        requires HasCoAwaitMethod<Awaitable>
    auto await_transform(Awaitable &&awaitable)
    {
        std::cout << "start to get Awaiter in Task:"<<_tcb.tid << std::endl;
        return std::forward<Awaitable>(awaitable).coAwait();
    }
    void return_value()
    {
    }

    void unhandled_exception() noexcept
    {
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
    std::coroutine_handle<promise_type> h_;

};

Task TaskPromise::get_return_object() noexcept
{
    return Task(std::coroutine_handle<Task::promise_type>::from_promise(*this));
}
