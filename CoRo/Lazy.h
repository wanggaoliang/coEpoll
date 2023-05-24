#pragma once

#include <atomic>
#include <concepts>
#include <cstdio>
#include <exception>
#include <variant>
#include <coroutine>
#include "../utils/NonCopyable.h"
template <typename T>
class Lazy;

template <class T>
concept HasCoAwaitMethod = requires(T &&awaitable) {
    std::forward<T>(awaitable).coAwait(0);
};

class LazyPromiseBase
{
public:
    struct FinalAwaiter
    {
        bool await_ready() const noexcept { return false; }

        template <typename PromiseType>
        auto await_suspend(std::coroutine_handle<PromiseType> h) noexcept
        {
            return h.promise()._continuation;
        }

        void await_resume() noexcept {}
    };

public:
    LazyPromiseBase() : _tid(0) {}

    std::suspend_always initial_suspend() noexcept { return {}; }

    FinalAwaiter final_suspend() noexcept { return {}; }

    template <typename Awaitable>
        requires HasCoAwaitMethod<Awaitable>
    auto await_transform(Awaitable &&awaitable)
    {
            return std::forward<Awaitable>(awaitable).coAwait(_tid);
    }

    std::coroutine_handle<> _continuation;
    int _tid;
};

template <typename T>
class LazyPromise : public LazyPromiseBase
{
public:
    LazyPromise() noexcept {}
    ~LazyPromise() noexcept {}

    Lazy<T> get_return_object() noexcept;

    template <typename V>
    void return_value(V &&value) noexcept(std::is_nothrow_constructible_v<T, V &&>)
        requires std::is_convertible_v<V &&, T>
    {
        _value.template emplace<T>(std::forward<V>(value));
    }

    void unhandled_exception() noexcept
    {
        _value.template emplace<std::exception_ptr>(std::current_exception());
    }

public:
    T &result() &
    {
        if (std::holds_alternative<std::exception_ptr>(_value))
            AS_UNLIKELY{
                std::rethrow_exception(std::get<std::exception_ptr>(_value));
        }
        assert(std::holds_alternative<T>(_value));
        return std::get<T>(_value);
    }
    T &&result() &&
    {
        if (std::holds_alternative<std::exception_ptr>(_value))
            AS_UNLIKELY{
                std::rethrow_exception(std::get<std::exception_ptr>(_value));
        }
        assert(std::holds_alternative<T>(_value));
        return std::move(std::get<T>(_value));
    }

    std::variant<std::monostate, T, std::exception_ptr> _value;
};

template <>
class LazyPromise<void> : public LazyPromiseBase
{
public:
    Lazy<void> get_return_object() noexcept;
    void return_void() noexcept {}
    void unhandled_exception() noexcept
    {
        _exception = std::current_exception();
    }

    void result()
    {
        if (_exception != nullptr)
            AS_UNLIKELY{ std::rethrow_exception(_exception); }
    }

public:
    std::exception_ptr _exception{ nullptr };
};

template <typename T>
struct ValueAwaiter : NonCopyable
{
    using Handle = CoroHandle<detail::LazyPromise<T>>;
    Handle _handle;
    int _tid;

    ValueAwaiter(Handle coro,int tid) : _hadnle(coro),_tid(tid) {}

    ValueAwaiter(ValueAwaiter &&other)
        : _handle(std::exchange(other._handle, nullptr)),_tid(other._tid)
    {}

    ~ValueAwaiter()
    {
        if (_handle)
        {
            _handle.destroy();
            _handle = nullptr;
        }
    }

    ValueAwaiter &operator=(ValueAwaiter &&other)
    {
        std::swap(_handle, other._handle);
        _tid = other._tid;
        return *this;
    }

    bool await_ready() const noexcept { return false; }

    AS_INLINE auto await_suspend(
        std::coroutine_handle<> continuation) noexcept
    {
        // current coro started, caller becomes my continuation
        this->_handle.promise()._continuation = continuation;
        this->_handle.promise()._tid = _tid;
        return this->_handle;
    }

    AS_INLINE T await_resume()
    {
        if constexpr (std::is_void_v<T>)
        {
            _handle.promise().result();
            // We need to destroy the handle expclictly since the awaited
            // coroutine after symmetric transfer couldn't release it self any
            // more.
            _handle.destroy();
            _handle = nullptr;
        }
        else
        {
            auto r = std::move(_handle.promise()).result();
            _handle.destroy();
            _handle = nullptr;
            return r;
        }
    }
};

template <typename T>
class Lazy :NonCopyable
{
public:
    using promise_type = detail::LazyPromise<T>;
    using Handle = CoroHandle<promise_type>;
    using ValueType = T;
    

    explicit Lazy(Handle coro) : _coro(coro) {};


    Lazy(Lazy &&other) : _coro(std::move(other._coro))
    {
        other._coro = nullptr;
    }

    ~Lazy()
    {
        if (_coro)
        {
            _coro.destroy();
            _coro = nullptr;
        }
    };

    Lazy& operator =(Lazy &&other)
    {
        _coro = std::move(other._coro);
        other._coro = nullptr;
    }

    bool isReady() const
    {
        return !_coro || _coro.done();
    }

    auto operator co_await(int tid)
    {
        return ValueAwaiter(std::exchange(_coro, nullptr),tid);
    }

    auto coAwait(int tid)
    {
        return ValueAwaiter(std::exchange(_coro, nullptr),tid);
    }
protected:
    Handle _coro;
};

template <typename T>
inline detail::Lazy<T> detail::LazyPromise<T>::get_return_object() noexcept
{
return Lazy<T>(Lazy<T>::Handle::from_promise(*this));
}

template <>
inline detail::Lazy<void> detail::LazyPromise<void>::get_return_object() noexcept
{
return Lazy<void>(Lazy<void>::Handle::from_promise(*this));
}

