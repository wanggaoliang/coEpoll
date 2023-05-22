#pragma once
#include "Common.h"
#include "../utils/LockFreeQueue.h"
#include <atomic>
#include <list>
class CoMutex
{
public:
    CoMutex();
    ~CoMutex();
    void lock()
    {
        auto w = waiter++;
        if (w)
        {
            
        }
    }
    
    void unlock()
    {
        auto w = waiter--;
        if (w != 1)
        {
            while ();
        }
    }
    
    uint getIndex() const;
private:
    std::atomic<int> waiter; 
    int hstart;
    uint index;
    MpscQueue<WQCallback> items_;
};