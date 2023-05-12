#pragma once
#include <type_traits>
#include <functional>
using WQCallback = std::function<void()>;
using WakeCallback = std::function<void(WQCallback &&)>;
using FDCallback = std::function<void(int)>;

template<typename T>
concept WQCallbackType = std::is_same_v<T, WQCallback>;

template<typename T>
concept WakeCallbackType = std::is_same_v<T, WakeCallback>;

template<typename T>
concept FDCallbackType = std::is_same_v<T, FDCallback>;