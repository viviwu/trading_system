/**
  ******************************************************************************
  * @file           : Channel.h
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#ifndef TRADING_SYSTEM_CHANNEL_H
#define TRADING_SYSTEM_CHANNEL_H

#pragma once
#include <functional>
#include <cstdint>

class EventLoop;

class Channel {
    EventLoop* loop_;
    int fd_;
    uint32_t events_{0};

    std::function<void()> readCb_;
    std::function<void()> writeCb_;

public:
    Channel(EventLoop* loop, int fd);

    int fd() const { return fd_; }
    uint32_t events() const { return events_; }

    void setReadCallback(std::function<void()> cb) { readCb_ = std::move(cb); }
    void setWriteCallback(std::function<void()> cb) { writeCb_ = std::move(cb); }

    void enableRead();
    void enableWrite();
    void disableWrite();

    void handle(uint32_t revents);
};


#endif //TRADING_SYSTEM_CHANNEL_H
