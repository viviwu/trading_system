/**
  ******************************************************************************
  * @file           : Protocol.h
  * @author         : vivi wu 
  * @brief          : 证券交易协议定义
  * @version        : 0.2.0
  * @date           : 30/04/26
  ******************************************************************************
  */

#ifndef TRADING_SYSTEM_PROTOCOL_H
#define TRADING_SYSTEM_PROTOCOL_H

#pragma once
#include <cstdint>

#pragma pack(push,1)

// 订单请求
struct OrderReq {
    uint32_t len;        // 包总长度
    uint64_t orderId;    // 订单ID
    uint32_t account;    // 账户ID
    double   price;      // 价格
    uint32_t qty;        // 数量
    uint8_t  side;       // 1=买(Buy), 2=卖(Sell)
    uint64_t ts;         // 客户端时间戳
    char     symbol[8];  // 交易对，如 "BTC/USD"
};

// 订单响应
// status: 0=全部成交, 1=部分成交, 2=风控拒绝, 3=未成交(已挂单)
struct OrderResp {
    uint32_t len;        // 包总长度
    uint64_t orderId;    // 订单ID
    uint8_t  status;     // 状态
    uint32_t filledQty;  // 已成交数量
    double   fillPrice;  // 成交均价
    uint64_t ts;         // 服务端时间戳
};

#pragma pack(pop)

#endif //TRADING_SYSTEM_PROTOCOL_H
