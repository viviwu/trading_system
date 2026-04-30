/**
 ******************************************************************************
 * @file           : OrderBook.h
 * @author         : vivi wu
 * @brief          : None
 * @version        : 0.1.0
 * @date           : 30/04/26
 ******************************************************************************
 */


#ifndef TRADING_SYSTEM_ORDERBOOK_H
#define TRADING_SYSTEM_ORDERBOOK_H


#pragma once
#include <deque>
#include <map>

struct Order {
  uint64_t id;
  double price;
  uint32_t qty;
  uint8_t side;
};

class OrderBook {
  // price → FIFO queue
  // std::map<double, std::deque<Order>> bids; // 买：大到小
  std::map<double, std::deque<Order>, std::greater<>> bids;
  std::map<double, std::deque<Order>> asks;  // 卖：小到大

 public:
  void match(Order& ord);
};


#endif  // TRADING_SYSTEM_ORDERBOOK_H
