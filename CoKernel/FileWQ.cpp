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

    for (auto iter = items_.begin(); iter != items_.end(); )
    {
            if (wakeCallback_)
            {
                wakeCallback_(iter->h_);
            }
            iter = items_.erase(iter);
    }
}

void FileWQ::wakeup()
{
    if (fdCallbck_)
    {
        fdCallbck_(fd_);
    }

    for (auto iter = items_.begin(); iter != items_.end(); )
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
                wakeCallback_(iter->h_);
            }
            iter = items_.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}