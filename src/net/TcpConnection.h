/**
  ******************************************************************************
  * @file           : TcpConnection.h
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#ifndef TRADING_SYSTEM_TCPCONNECTION_H
#define TRADING_SYSTEM_TCPCONNECTION_H


#pragma once
#include "Buffer.h"
#include <memory>

class EventLoop;
class Channel;
class MatchingEngine;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
    int fd_;
    EventLoop* loop_;
    std::unique_ptr<Channel> channel_;

    Buffer input_;
    Buffer output_;

    MatchingEngine* engine_;

public:
    TcpConnection(int fd, EventLoop* loop, MatchingEngine* engine);

    void establish();

    void handleRead();
    void handleWrite();
    void handleClose();

    void send(const char* data, size_t len);

    int fd() const { return fd_; }
};



#endif //TRADING_SYSTEM_TCPCONNECTION_H
