#pragma once
#include <coroutine>
#include <atomic>
#include "../utils/LockFreeQueue.h"
struct MuCore
{
    struct WaitItem
    {
        int tid_;
        std::coroutine_handle<> h_;

    };
    std::atomic<int> tid_;
    std::atomic<uint> waiter_;
    uint index;
    MpscQueue<WaitItem> items_;
};