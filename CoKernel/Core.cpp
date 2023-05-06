#include "Core.h"
#include <assert.h>
#include <functional>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

const int kPollTimeMs = 10000;

Core::Core()
    :poller_(new EPoller())
{
    
}


Core::~Core()
{
}


void Core::run()
{
    while (true)
    {
        while (!readyWQ_.empty())
        {
            auto func = readyWQ_.front();
            func();
            readyWQ_.pop();
        }
        
        poller_->wait(kPollTimeMs);
    }
}

void Core::updateFileWQ(int fd, uint32_t events)
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

void Core::removeFileWQ(int fd)
{
    auto file = fileWQPtrs_.find(fd);
    if (file != fileWQPtrs_.end())
    {
        poller_->updateWQ(EPOLL_CTL_MOD, file->second.get());
        fileWQPtrs_.erase(fd);
    }
}


void Core::schedule(WQCallback &&cb)
{
    readyWQ_.emplace(std::move(cb));
}
