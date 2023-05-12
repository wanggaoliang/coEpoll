#pragma once
#include "Common.h"
#include <atomic>
#include <list>
class CoMutex
{
public:
    CoMutex();
    ~CoMutex();
    void lock();
    void unlock();
    uint getIndex() const;
private:
    std::atomic<bool> flag;
    uint index;
    std::list<WQCallback> items_;
};