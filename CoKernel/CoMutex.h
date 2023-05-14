#pragma once
#include "Common.h"
#include "../utils/SpinLock.h"
#include <atomic>
#include <list>
class CoMutex
{
public:
    CoMutex();
    ~CoMutex();
    void lock()
    {
        auto cs = state.fetch_or(0x01);
        if (!(cs & LMask))
        {
            return;
        }
        if (cs & SMask || cs & WMask)
        {
            state.fetch_add(0x00000008);
            //add wait
            return;
        }
        
        if (now-past>1)
        {
            state.fetch_or(0x02);
            state.fetch_add(0x00000008);
            //add wait
            return;
        }
        
        while (hstart)
        {
            auto s = state.fetch_and(~0x04);
        }
    }
    void unlock();
    uint getIndex() const;
private:
    const int LMask = 0x01;
    const int SMask = 0x02;
    const int WMask = 0x04;
    const int RMask = 0xFFFFFFF8;
    std::atomic<int> state; /* |3-31 routine|2 woken|1 starve|0 locked|*/
    int hstart;
    SpinLock slk;
    uint index;
    std::list<WQCallback> items_;
    int past;
    int now;
    bool
};