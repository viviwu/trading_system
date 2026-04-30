/**
  ******************************************************************************
  * @file           : OrderBook.h
  * @author         : vivi wu
  * @brief          : 订单簿（每个 symbol 一个实例）
  * @version        : 0.2.0
  * @date           : 30/04/26
  ******************************************************************************
  */

#ifndef TRADING_SYSTEM_ORDERBOOK_H
#define TRADING_SYSTEM_ORDERBOOK_H

#pragma once
#include <deque>
#include <map>
#include <vector>
#include <cstdint>

struct Order {
    uint64_t id;
    double price;
    uint32_t qty;
    uint8_t side;
};

struct Fill {
    uint64_t takerId;   // 主动成交方订单ID
    uint64_t makerId;   // 被动挂单方订单ID
    double price;       // 成交价格（以 maker 价格成交）
    uint32_t qty;       // 成交数量
};

class OrderBook {
    // 买方订单簿：价格从高到低
    std::map<double, std::deque<Order>, std::greater<>> bids_;
    // 卖方订单簿：价格从低到高
    std::map<double, std::deque<Order>> asks_;

public:
    /**
     * @brief 撮合订单
     * @param ord 输入订单（qty 会被修改为剩余未成交数量）
     * @return 成交明细列表
     */
    std::vector<Fill> match(Order& ord);

    /**
     * @brief 获取买方最优价格（最高买价）
     */
    double bestBid() const {
        if (bids_.empty()) return 0.0;
        return bids_.begin()->first;
    }

    /**
     * @brief 获取卖方最优价格（最低卖价）
     */
    double bestAsk() const {
        if (asks_.empty()) return 0.0;
        return asks_.begin()->first;
    }

    /**
     * @brief 获取买方档位数量
     */
    size_t bidLevels() const { return bids_.size(); }

    /**
     * @brief 获取卖方档位数量
     */
    size_t askLevels() const { return asks_.size(); }
};

#endif  // TRADING_SYSTEM_ORDERBOOK_H
