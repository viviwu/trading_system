/**
  ******************************************************************************
  * @file           : EventLoop.cpp
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#include "EventLoop.h"
#include "Channel.h"
#include <unistd.h>
#include <vector>

EventLoop::EventLoop() {
    epfd = epoll_create1(0);
}

EventLoop::~EventLoop() {
    close(epfd);
}

void EventLoop::updateChannel(Channel* ch) {
    epoll_event ev{};
    ev.data.fd = ch->fd();
    ev.events = ch->events();

    if (channels.count(ch->fd()) == 0) {
        epoll_ctl(epfd, EPOLL_CTL_ADD, ch->fd(), &ev);
        channels[ch->fd()] = ch;
    } else {
        epoll_ctl(epfd, EPOLL_CTL_MOD, ch->fd(), &ev);
    }
}

void EventLoop::removeChannel(Channel* ch) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, ch->fd(), nullptr);
    channels.erase(ch->fd());
}

void EventLoop::loop() {
    std::vector<epoll_event> events(1024);

    while (true) {
        int n = epoll_wait(epfd, events.data(), events.size(), -1);

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            auto it = channels.find(fd);
            if (it != channels.end()) {
                it->second->handle(events[i].events);
            }
        }
    }
}
