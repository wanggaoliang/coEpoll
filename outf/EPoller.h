#ifndef EPOLL_H
#pragma once

#include <sys/epoll.h>
#include <vector>

class EPoller
{
public:
    EPoller();
    ~EPoller();

    void add(int fd, uint32_t events);
    void modify(int fd, uint32_t events);
    void remove(int fd);
    int wait(std::vector<epoll_event> &events, int timeout);

private:
    int epoll_fd_;
};