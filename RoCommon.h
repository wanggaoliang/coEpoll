#pragma once
#include <concepts>
#include <coroutine>
#include <memory>
#include <iostream>
#include "./utils/NonCopyable.h"
#include "./utils/Attribute.h"

struct UTCB
{
    int tid;
    void *core;
    UTCB(int t,void* c):tid(t),core(c){}
};

template<typename Awaitable>
concept HasCoAwaitMethod = requires(Awaitable && awaitable)
{
    std::forward<Awaitable>(awaitable).coAwait();
};

template<typename PromiseType>
concept HasUTCBStar = requires(PromiseType a)
{
    {a._tcb}->std::same_as<UTCB*&>;
};

template<typename PromiseType>
concept HasUTCBStruct = requires(PromiseType a)
{
    {a._tcb}->std::same_as<UTCB&>;
};

template<typename PromiseType>
concept HasUTCB = HasUTCBStruct<PromiseType> || HasUTCBStar<PromiseType>;