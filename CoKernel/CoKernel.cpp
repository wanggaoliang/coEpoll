#include "CoKernel.h"

void CoKernel::updateFileWQ(int fd, uint32_t events)
{
    auto file = fileWQPtrs_.find(fd);
    if (file == fileWQPtrs_.end())
    {
        auto [it, flag] = fileWQPtrs_.insert(FileWQMap::value_type(fd, std::make_shared<FileWQ>(fd)));
        it->second->setWEvents(events);
        auto core = thCores_.top();
        //core.addlisten(it->second.get())
        thCores_.pop();
        thCores_.emplace(core);
    }
    else
    {
        file->second->setWEvents(events);
        auto core = static_cast<Core *>(file->second->getArg());
        //core.modlisten(file->second.get());
    }
}

void CoKernel::removeFileWQ(int fd)
{
    auto file = fileWQPtrs_.find(fd);
    if (file != fileWQPtrs_.end())
    {
        auto core = static_cast<Core *>(file->second->getArg());
        //core.removelisten(it->second.get())
        fileWQPtrs_.erase(fd);
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

Core* CoKernel::getCore()
{
    std::call_once(once_, [this]() {
        this->core_ = std::make_shared<Core>(this);
                   });
    return core_.get();
}