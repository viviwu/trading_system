/**
  ******************************************************************************
  * @file           : EventLoop.h
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#ifndef TRADING_SYSTEM_EVENTLOOP_H
#define TRADING_SYSTEM_EVENTLOOP_H

#pragma once
#include <sys/epoll.h>
#include <unordered_map>

class Channel;

class EventLoop {
    int epfd;
    std::unordered_map<int, Channel*> channels;

public:
    EventLoop();
    ~EventLoop();

    void loop();

    void updateChannel(Channel* ch);
    void removeChannel(Channel* ch);
};


#endif //TRADING_SYSTEM_EVENTLOOP_H
