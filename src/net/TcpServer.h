/**
  ******************************************************************************
  * @file           : TcpServer.h
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#ifndef TRADING_SYSTEM_TCPSERVER_H
#define TRADING_SYSTEM_TCPSERVER_H



#pragma once
#include <memory>
#include <unordered_map>

class EventLoop;
class TcpConnection;
class MatchingEngine;

class TcpServer {
    int listenfd_;
    EventLoop* loop_;
    MatchingEngine* engine_;

    std::unordered_map<int, std::shared_ptr<TcpConnection>> conns_;

public:
    TcpServer(int port, MatchingEngine* engine);
    void start();

    void handleAccept();
};


#endif //TRADING_SYSTEM_TCPSERVER_H
