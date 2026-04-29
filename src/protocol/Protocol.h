/**
  ******************************************************************************
  * @file           : Protocol.h
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#ifndef TRADING_SYSTEM_PROTOCOL_H
#define TRADING_SYSTEM_PROTOCOL_H

#pragma once
#include <cstdint>

#pragma pack(push,1)

struct OrderReq {
    uint32_t len;
    uint64_t orderId;
    uint32_t account;
    double   price;
    uint32_t qty;
    uint8_t  side;
    uint64_t ts;
};

struct OrderResp {
    uint32_t len;
    uint64_t orderId;
    uint8_t  status;
    uint32_t filledQty;
    double   fillPrice;
    uint64_t ts;
};

#pragma pack(pop)

#endif //TRADING_SYSTEM_PROTOCOL_H
