#pragma once
#include <sys/epoll.h>
#include "WQAbstract.h"

class FDICU
{
public:
    FDICU();
    ~FDICU();

    void updateIRQ(int, WQAbstract*);
    int waitIRQ(int);

private:
    int epoll_fd_;
    epoll_event events_[128];
};