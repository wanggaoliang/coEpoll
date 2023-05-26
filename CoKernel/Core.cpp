#include "Core.h"
#include "../utils/ScopeExit.h"
#include <assert.h>
#include <sys/eventfd.h>
#include <signal.h>
#include <fcntl.h>

const int kPollTimeMs = 10000;

Core::Core()
    :fdICU_(new FDICU()),
    looping_(false),
    quit_(false),
    threadId_(std::this_thread::get_id())
{
    int wakeupFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeupFd < 0)
    {
    }
    wakeUpWQ_.reset(new FileWQ(wakeupFd, this));
    wakeUpWQ_->setFDCallback(std::function([](int fd) {
        uint64_t tmp;
        read(fd, &tmp, sizeof(tmp));}));
    fdICU_->updateIRQ(EPOLL_CTL_ADD, wakeUpWQ_.get());
    timerWQPtr_.reset(new TimeWQ(this));
    fdICU_->updateIRQ(EPOLL_CTL_ADD, timerWQPtr_.get());
}


Core::~Core()
{
    quit();

    while (looping_.load(std::memory_order_acquire))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}


void Core::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_.store(true, std::memory_order_release);
    quit_.store(false, std::memory_order_release);

    auto loopFlagCleaner = makeScopeExit(
        [this]() { looping_.store(false, std::memory_order_release); });
    while (!quit_.load(std::memory_order_acquire))
    {
        while (!funcs_.empty())
        {
            WQCallback func;
            while (funcs_.dequeue(func))
            {
                func();
            }
        }

        while (!readyRo_.empty())
        {
            auto h = readyRo_.front();
            h.resume();
            readyRo_.pop();
        }
        fdICU_->waitIRQ(kPollTimeMs);
    }
}

void Core::quit()
{
    quit_.store(true, std::memory_order_release);

    if (!isInLoopThread())
    {
        wakeUp();
    }
}

/* 跨线程 eventfd read/write 线程安全*/
void Core::wakeUp()
{
    uint64_t tmp = 1;
    write(wakeUpWQ_->getFd(), &tmp, sizeof(tmp));
}

void Core::assertInLoopThread()
{
    if (!isInLoopThread())
    {
        exit(1);
    }
}
