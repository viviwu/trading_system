/**
  ******************************************************************************
  * @file           : MatchingEngine.h
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#ifndef TRADING_SYSTEM_MATCHINGENGINE_H
#define TRADING_SYSTEM_MATCHINGENGINE_H


#pragma once
#include "OrderBook.h"
#include "../protocol/Protocol.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

class MatchingEngine {
    OrderBook book_;

    std::queue<std::pair<OrderReq, std::function<void(OrderResp)>>> q_;
    std::mutex mtx_;
    std::condition_variable cv_;

public:
    void start();

    void submit(const OrderReq&, std::function<void(OrderResp)>);
};


#endif //TRADING_SYSTEM_MATCHINGENGINE_H
