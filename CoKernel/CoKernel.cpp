#include "CoKernel.h"
#include "MuAwaiter.h"

CoKernel::CoKernel(uint num) :coreNum_(num)
{
    auto cPtr = std::make_shared<ThreadCore>(this);
    for (int i = 0;i < coreNum_;i++)
    {
        thCores_.push_back(cPtr);
        cores_.push_back(cPtr->getCore());
    }
    core_ = std::make_shared<Core>(this);
    cores_.push_back(core_.get());
    for (auto co : cores_)
    {
        if (co)
        {
            co->setPickUP(std::bind(&CoKernel::wakeUpReady, this));
        }
    }

    timerWQPtr_.reset(new TimeWQ(core_.get()));
    core_->addIRQ(static_cast<WQAbstract *>(timerWQPtr_.get()));
}

void CoKernel::start()
{
    for (auto th : thCores_)
    {
        th->run();
    }
    core_->loop();
}

Lazy<void> CoKernel::waitFile(int fd, uint32_t events)
{
    auto fileWQ = fileWQPtrs_.find(fd);

    if (fileWQ == fileWQPtrs_.end())
    {
        return;
    }

    fileWQ->second->addWait(std::forward<T>(cb), events);
}

Lazy<void> CoKernel::waitTime(const TimeWQ::TimePoint &tp)
{
    if (!timerWQPtr_)
    {
        timerWQPtr_.reset(new TimeWQ());
    }
    timerWQPtr_->addWait(std::forward<T>(cb), tp);
}

Lazy<void> CoKernel::CoRoLock(MuCore &mu)
{
    co_await LockAwaiter{ &mu };
}

Lazy<void> CoKernel::CoRoUnlock(MuCore &)
{
    co_await UnlockAwaiter{ &mu ,std::bind(&CoKernel::schedule,this) };
}

Lazy<int> CoKernel::updateIRQ(int fd, uint32_t events)
{
    Core *core = nullptr;
    fqlk.lock();
    auto file = fileWQPtrs_.find(fd);
    auto found = file != fileWQPtrs_.end();
    if (!found)
    {
        core = getNextCore();
    }
    else
    {
        core = static_cast<Core *>(file->second->getCore());
    }
    auto ret = co_await ReqIRQRet{ core,[fd,events,this]() -> int {
        return this->updateFileWQ(fd,events);
    } };
    fqlk.unlock();
    co_return ret;

}

CoKernel::ReqIRQRet CoKernel::removeIRQ(int fd)
{
    return ReqIRQRet{ core_.get(),[fd,this]() -> int {
        return this->removeFileWQ(fd);
    } };
}

void CoKernel::schedule(std::coroutine_handle<> &h)
{
    readyWQ_.emplace(h);
}

void CoKernel::wakeUpReady()
{
    while (!readyWQ_.empty())
    {
        auto func = readyWQ_.front();
        func();
        readyWQ_.pop();
    }
}

Core *CoKernel::getNextCore()
{
    auto index = nextCore_.fetch_add(1, std::memory_order_acq_rel);
    return cores_[index];
}