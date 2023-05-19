#include "CoKernel.h"

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

CoKernel::ReqIRQRet CoKernel::updateIRQ(int fd, uint32_t events)
{
    auto file = fileWQPtrs_.find(fd);
    auto found = file != fileWQPtrs_.end();
    auto core = found ? file->second.get() : nullptr;
    fqlk.unlock();
    return ReqIRQRet{ core_.get(),[fd,events,this]() -> int {
        return this->updateFileWQ(fd,events);
    }};
    
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