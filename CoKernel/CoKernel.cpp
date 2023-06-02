#include "CoKernel.h"
#include "MuAwaiter.h"

CoKernel::CoKernel(uint num) :coreNum_(num)
{
    for (int i = 0;i < coreNum_;i++)
    {
        auto cPtr = std::make_shared<ThreadCore>();
        thCores_.push_back(cPtr);
        irqs_.push_back(cPtr->getCore());
    }
    core_ = std::make_shared<Core>();
    irqs_.push_back(core_.get());
    for (auto irq : irqs_)
    {
        if (irq)
        {
            irq->setPickUP(std::bind(&CoKernel::wakeUpReady, this));
        }
    }
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
    co_return;
}

Lazy<void> CoKernel::waitTime(const TimePoint &tp)
{
    co_await TimeAwaiter{ tp,Core::getCurCore() };
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
    int ret = 0;
    int i = 0;
    co_await CoRoLock(fMapLk_);
    auto file = fileWQPtrs_.find(fd);
    auto found = file != fileWQPtrs_.end();
    if (!found)
    {
        core = irqs_[0];
        auto fq = std::make_shared<FileWQ>(fd, core);
        fq->setWEvents(events);
        ret = co_await ReqIRQRet{ core,[core, fq ,this]() -> int {
        return core->addIRQ(fq.get());
        }};
        if (!ret)
        {
            fileWQPtrs_.insert(FileWQMap::value_type{fd, fq});
            
            for (i=1;i < coreNum_;i++)
            {
                if (irqs_[i]->irqn > irqs_[0]->irqn)
                {
                    break;
                }
            }
            irqs_[0]->irqn++;
            if (i != 1)
            {
                auto temp = irqs_[0];
                irqs_[0] = irqs_[i - 1];
                irqs_[0]->pos = i - 1;
                irqs_[i - 1] = temp;
                irqs_[i - 1]->pos = 0;
            }
        }
    }
    else
    {
        core = static_cast<Core *>(file->second->getCore());
        file->second->setWEvents(events);
        ret = co_await ReqIRQRet{ core,[core, fq = file->second,this]() -> int {
        return core->modIRQ(fq.get());
        } };
        
    }
    co_await CoRoUnlock(fMapLk_);
    co_return ret;
}

Lazy<int> CoKernel::removeIRQ(int fd)
{
    Core *core = nullptr;
    int ret = 0;
    int i = 0;
    co_await CoRoLock(fMapLk_);
    auto file = fileWQPtrs_.find(fd);
    auto found = file != fileWQPtrs_.end();
    if (found)
    {
        core = static_cast<Core*>(file->second->getCore());
        ret = co_await ReqIRQRet{ core,[core, fq = file->second,this]() -> int {
        return core->delIRQ(fq.get());
        } };
        fileWQPtrs_.erase(fd);

        for (i = core->pos - 1;i >= 0;i--)
        {
            if (irqs_[i]->irqn < irqs_[core->pos]->irqn)
            {
                break;
            }
        }
        irqs_[core->pos]->irqn--;
        if (i != core->pos - 1)
        {
            auto temp = irqs_[core->pos];
            irqs_[core->pos] = irqs_[i + 1];
            irqs_[i + 1] = temp;
            irqs_[i + 1]->pos = core->pos;
            core->pos = i + 1;
        }
    }
    co_await CoRoUnlock(fMapLk_);
    co_return ret;
;
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

{
    auto index = nextCore_.fetch_add(1, std::memory_order_acq_rel);
    return cores_[index];
}