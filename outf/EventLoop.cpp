#include "EventLoop.h"
#include <assert.h>
#include <iostream>
#include <functional>
#include <unistd.h>
#include <algorithm>
#include <signal.h>
#include <fcntl.h>

const int kPollTimeMs = 10000;

EventLoop::EventLoop()
    : looping_(false),
    quit_(false),
    poller_(new EPoller()),
{
    
}


EventLoop::~EventLoop()
{
    struct timespec delay = {0, 1000000}; /* 1 msec */

    quit();

    // Spin waiting for the loop to exit because
    // this may take some time to complete. We
    // assume the loop thread will *always* exit.
    // If this cannot be guaranteed then one option
    // might be to abort waiting and
    // assert(!looping_) after some delay;
    while (looping_.load(std::memory_order_acquire))
    {
        nanosleep(&delay, nullptr);
    }
}

void EventLoop::quit()
{
    quit_.store(true, std::memory_order_release);

    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::run()
{
    assert(!looping_);
    looping_.store(true, std::memory_order_release);
    quit_.store(false, std::memory_order_release);

    auto loopFlagCleaner = makeScopeExit(
        [this]() { looping_.store(false, std::memory_order_release); });
    while (!quit_.load(std::memory_order_acquire))
    {
        poller_->wait(kPollTimeMs);
        while (!readyWQ_.empty())
        {
            auto func = readyWQ_.front();
            func();
            readyWQ_.pop();
        }
    }
}

void EventLoop::updateFileWQ(int fd, uint32_t events)
{
    auto file = fileWQPtrs_.find(fd);
    if (file == fileWQPtrs_.end())
    {
        auto [it, flag] = fileWQPtrs_.insert(FileWQMap::value_type(fd, std::make_shared<FileWQ>(fd)));
        it->second->setWEvents(events);
        poller_->updateWQ(EPOLL_CTL_ADD, it->second.get());
    }
    else
    {
        file->second->setWEvents(events);
        poller_->updateWQ(EPOLL_CTL_MOD, file->second.get());
    }
}

void EventLoop::removeFileWQ(int fd)
{
    auto file = fileWQPtrs_.find(fd);
    if (file != fileWQPtrs_.end())
    {
        poller_->updateWQ(EPOLL_CTL_MOD, file->second.get());
        fileWQPtrs_.erase(fd);
    }
}

TimerId EventLoop::runAt(const Date &time, const Func &cb)
{
    auto microSeconds =
        time.microSecondsSinceEpoch() - Date::now().microSecondsSinceEpoch();
    std::chrono::steady_clock::time_point tp =
        std::chrono::steady_clock::now() +
        std::chrono::microseconds(microSeconds);
    return timerQueue_->addTimer(cb, tp, std::chrono::microseconds(0));
}
TimerId EventLoop::runAt(const Date &time, Func &&cb)
{
    auto microSeconds =
        time.microSecondsSinceEpoch() - Date::now().microSecondsSinceEpoch();
    std::chrono::steady_clock::time_point tp =
        std::chrono::steady_clock::now() +
        std::chrono::microseconds(microSeconds);
    return timerQueue_->addTimer(std::move(cb),
                                 tp,
                                 std::chrono::microseconds(0));
}
TimerId EventLoop::runAfter(double delay, const Func &cb)
{
    return runAt(Date::date().after(delay), cb);
}

TimerId EventLoop::runAfter(double delay, Func &&cb)
{
    return runAt(Date::date().after(delay), std::move(cb));
}

TimerId EventLoop::runEvery(double interval, const Func &cb)
{
    std::chrono::microseconds dur(
        static_cast<std::chrono::microseconds::rep>(interval * 1000000));
    auto tp = std::chrono::steady_clock::now() + dur;
    return timerQueue_->addTimer(cb, tp, dur);
}
TimerId EventLoop::runEvery(double interval, Func &&cb)
{
    std::chrono::microseconds dur(
        static_cast<std::chrono::microseconds::rep>(interval * 1000000));
    auto tp = std::chrono::steady_clock::now() + dur;
    return timerQueue_->addTimer(std::move(cb), tp, dur);
}
void EventLoop::invalidateTimer(TimerId id)
{
    if (isRunning() && timerQueue_)
        timerQueue_->invalidateTimer(id);
}

