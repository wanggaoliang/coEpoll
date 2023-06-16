#pragma once
#include <type_traits>
#include <functional>
#include <coroutine>
using XCoreFunc= std::function<void()>;
using WakeCB = std::function<void(std::coroutine_handle<> &)>;
using ScheCB = std::function<bool()>;
