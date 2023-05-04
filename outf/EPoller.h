#pragma once
#include <sys/epoll.h>
#include "WQAbstract.h"

class EPoller
{
public:
    EPoller();
    ~EPoller();

    void updateWQ(int, WQAbstract*);
    int wait(int timeout);

private:
    int epoll_fd_;
    epoll_event events_[128];
};