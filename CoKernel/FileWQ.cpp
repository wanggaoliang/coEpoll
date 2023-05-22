#include "FileWQ.h"

FileWQ::FileWQ(int fd, void *core) :WQAbstract(fd, core)
{

}

FileWQ::~FileWQ()
{
    if (fdCallbck_)
    {
        fdCallbck_(fd_);
    }

    for (auto iter = items.begin(); iter != items.end(); )
    {
            if (wakeCallback_)
            {
                wakeCallback_(std::move(iter->func_));
            }
            iter = items.erase(iter);
    }
}

void FileWQ::wakeup()
{
    if (fdCallbck_)
    {
        fdCallbck_(fd_);
    }

    for (auto iter = items.begin(); iter != items.end(); )
    {
        auto tevent = removeREvents(~iter->events_);
        if (!tevent)
        {
            break;
        }
        else if (iter->events_ & tevent)
        {
            if (wakeCallback_)
            {
                wakeCallback_(std::move(iter->func_));
            }
            iter = items.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}