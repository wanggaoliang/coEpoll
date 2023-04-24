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

void EPoller::add(int fd, uint32_t events)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event);
}

void EPoller::modify(int fd, uint32_t events)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
}

void EPoller::remove(int fd)
{
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
}

int EPoller::wait(std::vector<epoll_event> &events, int timeout)
{
    int num_events = epoll_wait(epoll_fd_, events.data(), events.size(), timeout);
    if (num_events < 0)
    {
        return num_events;
    }
    events.resize(num_events);
    return num_events;
}