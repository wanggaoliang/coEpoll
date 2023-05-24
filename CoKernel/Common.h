#pragma once
#include <type_traits>
#include <functional>
#include <coroutine>
using WQCallback = std::function<void()>;
using TWakeCallback = std::function<void(WQCallback &&)>;
using FDCallback = std::function<void(int)>;
using WakeCallback = std::function<void(std::coroutine_handle<>&)>;