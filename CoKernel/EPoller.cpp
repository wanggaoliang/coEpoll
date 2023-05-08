#include "EPoller.h"
#include <unistd.h>

EPoller::EPoller()
{
    epoll_fd_ = epoll_create1(0);
}

EPoller::~EPoller()
{
    ::close(epoll_fd_);
}

void EPoller::updateWQ(int op, WQAbstract *WQA)
{
    epoll_event event;
    event.data.ptr = WQA;
    event.events = WQA->addWEvents(0);
    epoll_ctl(epoll_fd_, op, WQA->getFd(), &event);
}

int EPoller::wait(int timeout)
{
    int num_events = epoll_wait(epoll_fd_, events_, 128, timeout);
    if (num_events < 0)
    {
        return num_events;
    }
    for (int i = 0;i < num_events;i++)
    {
        WQAbstract *WQA = static_cast<WQAbstract *>(events_[i].data.ptr);
        WQA->addREvents(events_[i].events);
        WQA->wakeup();
    }
    return num_events;
}
