#pragma once
#include <type_traits>
#include <functional>
class WQAbstract
{
public:
    WQAbstract() {};
    ~WQAbstract() {};
    virtual void wakeup(uint32_t) = 0;
};

using WQCallback = std::function<void()>;

template<typename T>
concept IsWQCallback = std::is_same_v<T, WQCallback>;