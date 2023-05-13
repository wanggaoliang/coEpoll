#include "CoKernel.h"

CoKernel::ReqIRQRet CoKernel::updateFileWQ(int fd, uint32_t events)
{
    return ReqIRQRet{core_.get(),[]() -> int {

    }};
    fqlk.lock();
    auto file = fileWQPtrs_.find(fd);
    auto found = file != fileWQPtrs_.end();
    /* fd 的创建是线程安全的因此在没有执行remove前不可能add两个相同的fd，remove前必定已add,因此两个add无竞争条件*/
    if (!found)
    {
        fqlk.unlock();
        auto fq = std::make_shared<FileWQ>(fd);
        fq->setWEvents(events);
        auto core = getNextCore();
        core->addIRQ(fq.get());
        fqlk.lock();
        fileWQPtrs_.insert(FileWQMap::value_type(fd, std::make_shared<FileWQ>(fd)));
        fqlk.unlock();
    }
    else
    {
        file->second->setWEvents(events);
        auto core = static_cast<Core *>(file->second->getArg());
        fqlk.unlock();
        core->modIRQ(file->second.get());
    }
}

void CoKernel::removeFileWQ(int fd)
{
    auto file = fileWQPtrs_.find(fd);
    if (file != fileWQPtrs_.end())
    {
        auto core = static_cast<Core *>(file->second->getArg());
        core->removeIRQ(file->second.get());
    }
}

void CoKernel::schedule(WQCallback &&cb)
{
    readyWQ_.emplace(std::move(cb));
}

void CoKernel::wakeUpReady()
{
    while (!readyWQ_.empty())
    {
        readyWQ_.top();
    }
}

Core* CoKernel::getBPCore()
{
    std::call_once(once_, [this]() {
        this->core_ = std::make_shared<Core>(this);
                   });
    return core_.get();
}

Core *CoKernel::getNextCore()
{
    auto index = nextCore_.fetch_add(1, std::memory_order_acq_rel);
    return cores_[index];
}