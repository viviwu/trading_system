/**
  ******************************************************************************
  * @file           : OrderBook.cpp
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#include "OrderBook.h"

void OrderBook::match(Order& ord) {
    if (ord.side == 1) {
        while (ord.qty > 0 && !asks.empty()) {
            auto it = asks.begin();
            if (it->first > ord.price) break;

            auto& q = it->second;
            auto& top = q.front();

            uint32_t fill = std::min(ord.qty, top.qty);
            ord.qty -= fill;
            top.qty -= fill;

            if (top.qty == 0) q.pop_front();
            if (q.empty()) asks.erase(it);
        }

        if (ord.qty > 0) bids[ord.price].push_back(ord);
    } else {
        while (ord.qty > 0 && !bids.empty()) {
            auto it = bids.begin();
            if (it->first < ord.price) break;

            auto& q = it->second;
            auto& top = q.front();

            uint32_t fill = std::min(ord.qty, top.qty);
            ord.qty -= fill;
            top.qty -= fill;

            if (top.qty == 0) q.pop_front();
            if (q.empty()) bids.erase(it);
        }

        if (ord.qty > 0) asks[ord.price].push_back(ord);
    }
}