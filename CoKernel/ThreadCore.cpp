#include "ThreadCore.h"
ThreadCore::ThreadCore()
    :thread_([this]() { loopFuncs(); })
{
    auto f = promiseForCorePtr_.get_future();
    core_ = f.get();
}

ThreadCore::~ThreadCore()
{
    run();
    std::shared_ptr<Core> core;
    {
        std::unique_lock<std::mutex> lk(loopMutex_);
        core = core_;
    }
    if (core)
    {
        core->quit();
    }
    if (thread_.joinable())
    {
        thread_.join();
    }
}

void ThreadCore::wait()
{
    thread_.join();
}

void ThreadCore::loopFuncs()
{

    thread_local static std::shared_ptr<Core> core =
        std::make_shared<Core>();
    core->bindCore2Thread();
    core->queueInLoop([this]() { promiseForLoop_.set_value(1); });
    promiseForCorePtr_.set_value(core);
    auto f = promiseForRun_.get_future();
    (void) f.get();
    core->loop();
    {
        std::unique_lock<std::mutex> lk(loopMutex_);
        core_ = nullptr;
    }
}

void ThreadCore::run()
{
    std::call_once(once_, [this]() {
        auto f = promiseForLoop_.get_future();
        promiseForRun_.set_value(1);
        f.get();});
}
