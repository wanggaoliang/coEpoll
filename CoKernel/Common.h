#pragma once
#include <type_traits>
#include <functional>
#include <coroutine>
using XCoreFunc= std::function<void()>;
using WakeCallback = std::function<void(std::coroutine_handle<> &)>;