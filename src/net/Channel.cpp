/**
  ******************************************************************************
  * @file           : Channel.cpp
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#include "Channel.h"
#include "EventLoop.h"
#include <sys/epoll.h>

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop), fd_(fd) {}

void Channel::enableRead() {
    events_ |= EPOLLIN | EPOLLET;
    loop_->updateChannel(this);
}

void Channel::enableWrite() {
    events_ |= EPOLLOUT | EPOLLET;
    loop_->updateChannel(this);
}

void Channel::disableWrite() {
    events_ &= ~EPOLLOUT;
    loop_->updateChannel(this);
}

void Channel::handle(uint32_t revents) {
    if ((revents & EPOLLIN) && readCb_) readCb_();
    if ((revents & EPOLLOUT) && writeCb_) writeCb_();
}
