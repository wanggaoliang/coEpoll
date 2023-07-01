#include "Core.h"
#include "../utils/ScopeExit.h"
#include <assert.h>
#include <sys/eventfd.h>
#include <signal.h>
#include <fcntl.h>

const int kPollTimeMs = 1000;
uint Core::coreNum = 0;

thread_local Core *Core::curCore = nullptr;

Core::Core()
    :fdICU_(new FDICU()),
    looping_(false),
    quit_(false),
    threadId_(std::this_thread::get_id()),
    index(coreNum++),
    irqn(0),
    pos(index)
{
    int wakeupFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeupFd < 0)
    {
    }
    wakeUpWQ_.reset(new FileWQ(wakeupFd, this));
    wakeUpWQ_->setFDCallback(std::function([](int fd) {
        uint64_t tmp;
        read(fd, &tmp, sizeof(tmp));}));
    wakeUpWQ_->setWEvents(EPOLLIN | EPOLLET);
    fdICU_->updateIRQ(EPOLL_CTL_ADD, wakeUpWQ_.get());
    timerWQPtr_.reset(new TimeWQ());
    timerWQPtr_->setWEvents(EPOLLIN | EPOLLET);
    fdICU_->updateIRQ(EPOLL_CTL_ADD, timerWQPtr_.get());
}


Core::~Core()
{
    quit();

    while (looping_.load(std::memory_order_acquire))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ::close(wakeUpWQ_->getFd());
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
        XCoreFunc func;
        bool ret = false;
        do
        {
            ret = false;
            while (funcs_.dequeue(func))
            {
                func();
            }

            if (scheCB_)
            {
                ret = scheCB_();
            }
        } while (ret);
        
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
