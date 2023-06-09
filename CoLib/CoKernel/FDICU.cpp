#include "FDICU.h"
#include <unistd.h>
#include <errno.h>

FDICU::FDICU()
{
    epoll_fd_ = epoll_create1(0);
}

FDICU::~FDICU()
{
    ::close(epoll_fd_);
}

int FDICU::updateIRQ(int op, WQAbstract *WQA)
{
    epoll_event event;
    event.data.ptr = WQA;
    event.events = WQA->getWEvents();
    auto ret = ::epoll_ctl(epoll_fd_, op, WQA->getFd(), &event);
    if (ret < 0)
    {
        ret = -errno;
    }
    return ret;
}

int FDICU::waitIRQ(int timeout)
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
