/**
  ******************************************************************************
  * @file           : OrderBook.cpp
  * @author         : vivi wu 
  * @brief          : 订单簿撮合逻辑
  * @version        : 0.2.0
  * @date           : 30/04/26
  ******************************************************************************
  */

#include "OrderBook.h"
#include <algorithm>

std::vector<Fill> OrderBook::match(Order& ord) {
    std::vector<Fill> fills;
    fills.reserve(4);  // 大部分订单成交笔数很少

    if (ord.side == 1) {  // 买单：与卖方 asks 撮合
        while (ord.qty > 0 && !asks_.empty()) {
            auto it = asks_.begin();
            if (it->first > ord.price) break;  // 最优卖价高于买价，无法成交

            auto& q = it->second;
            auto& top = q.front();

            uint32_t fillQty = std::min(ord.qty, top.qty);
            ord.qty -= fillQty;
            top.qty -= fillQty;

            fills.push_back(Fill{ord.id, top.id, top.price, fillQty});

            if (top.qty == 0) q.pop_front();
            if (q.empty()) asks_.erase(it);
        }

        // 剩余未成交部分挂到买方订单簿
        if (ord.qty > 0) {
            bids_[ord.price].push_back(ord);
        }
    } else {  // 卖单：与买方 bids 撮合
        while (ord.qty > 0 && !bids_.empty()) {
            auto it = bids_.begin();
            if (it->first < ord.price) break;  // 最优买价低于卖价，无法成交

            auto& q = it->second;
            auto& top = q.front();

            uint32_t fillQty = std::min(ord.qty, top.qty);
            ord.qty -= fillQty;
            top.qty -= fillQty;

            fills.push_back(Fill{ord.id, top.id, top.price, fillQty});

            if (top.qty == 0) q.pop_front();
            if (q.empty()) bids_.erase(it);
        }

        // 剩余未成交部分挂到卖方订单簿
        if (ord.qty > 0) {
            asks_[ord.price].push_back(ord);
        }
    }

    return fills;
}
