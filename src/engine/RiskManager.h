/**
  ******************************************************************************
  * @file           : RiskManager.h
  * @author         : vivi wu 
  * @brief          : 风控管理（账户余额与持仓检查）
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */

#ifndef TRADING_SYSTEM_RISKMANAGER_H
#define TRADING_SYSTEM_RISKMANAGER_H

#pragma once
#include <cstdint>
#include <unordered_map>
#include "../protocol/Protocol.h"

struct Account {
    double balance{1e9};      // 可用资金
    uint32_t position{100000}; // 可用持仓
};

/**
 * @brief 风控管理器
 * 
 * 每个 MatchingEngineShard 内部持有一个 RiskManager 实例，
 * 利用 shard 单线程串行处理特性，天然线程安全。
 */
class RiskManager {
    std::unordered_map<uint32_t, Account> accounts_;

public:
    /**
     * @brief 检查订单是否满足风控条件
     * @return true 通过, false 拒绝
     */
    bool check(const OrderReq& ord) {
        auto& acc = accounts_[ord.account];

        if (ord.side == 1) {  // 买单：检查余额
            return acc.balance >= ord.price * ord.qty;
        } else {              // 卖单：检查持仓
            return acc.position >= ord.qty;
        }
    }

    /**
     * @brief 应用订单（冻结/扣减资金或持仓）
     */
    void apply(const OrderReq& ord) {
        auto& acc = accounts_[ord.account];

        if (ord.side == 1) {
            acc.balance -= ord.price * ord.qty;
        } else {
            acc.position -= ord.qty;
        }
    }

    /**
     * @brief 成交回滚/释放（部分成交后释放多余冻结）
     * @param ord 原始订单
     * @param filledQty 实际成交数量
     */
    void release(const OrderReq& ord, uint32_t filledQty) {
        auto& acc = accounts_[ord.account];
        uint32_t unfilled = ord.qty - filledQty;

        if (ord.side == 1) {
            // 买单：未成交部分释放资金
            acc.balance += ord.price * unfilled;
        } else {
            // 卖单：未成交部分释放持仓
            acc.position += unfilled;
        }
    }

    /**
     * @brief 查询账户信息（调试用）
     */
    Account getAccount(uint32_t accountId) const {
        auto it = accounts_.find(accountId);
        if (it != accounts_.end()) {
            return it->second;
        }
        return Account{};
    }
};

#endif // TRADING_SYSTEM_RISKMANAGER_H
