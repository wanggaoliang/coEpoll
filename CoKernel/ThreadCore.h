#pragma once
#include "Core.h"
#include <mutex>
#include <thread>
#include <future>
#include <memory>
class ThreadCore
{
public:
    ThreadCore();
    
    ~ThreadCore();
    
    void run();

    void wait();

    Core *getCore() const
    {
        return core_.get();
    }
    
    
private:
    void loopFuncs();
    std::shared_ptr<Core> core_;
    
    std::once_flag once_;
    std::thread thread_;
    std::mutex loopMutex_;

    std::promise<std::shared_ptr<Core>> promiseForCorePtr_;
    std::promise<int> promiseForRun_;
    std::promise<int> promiseForLoop_;
};

