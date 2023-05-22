#include "CoKernel.h"

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

int CoKernel::updateFileWQ(int fd, uint32_t events)
{
    int ret;
    fqlk.lock();
    auto file = fileWQPtrs_.find(fd);
    auto found = file != fileWQPtrs_.end();
    fqlk.unlock();
    /* fd 的创建是线程安全的因此在没有执行remove前不可能add两个相同的fd，remove前必定已add,因此两个add无竞争条件*/
    if (!found)
    {
        auto fq = std::make_shared<FileWQ>(fd);
        fq->setWEvents(events);
        auto core = getNextCore();
        ret = core->addIRQ(fq.get());
        fqlk.lock();
        fileWQPtrs_.insert(FileWQMap::value_type(fd, std::make_shared<FileWQ>(fd)));
        fqlk.unlock();
    }
    else
    {
        file->second->setWEvents(events);
        auto core = static_cast<Core *>(file->second->getArg());
        ret = core->modIRQ(file->second.get());
    }
    return ret;
}

int CoKernel::removeFileWQ(int fd)
{
    int ret = 0;
    fqlk.lock();
    auto file = fileWQPtrs_.find(fd);
    auto found = file != fileWQPtrs_.end();
    fqlk.unlock();
    if (found)
    {
        auto core = static_cast<Core *>(file->second->getArg());
        ret = core->removeIRQ(file->second.get());
        fqlk.lock();
        fileWQPtrs_.erase(fd);
        fqlk.unlock();
    }

    return ret;
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

void CoKernel::schedule(WQCallback &&cb)
{
    readyWQ_.emplace(std::move(cb));
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