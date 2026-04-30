/**
  ******************************************************************************
  * @file           : MatchingEngine.h
  * @author         : vivi wu 
  * @brief          : 全局撮合引擎（多 symbol 分片管理）
  * @version        : 0.2.0
  * @date           : 30/04/26
  ******************************************************************************
  */

#ifndef TRADING_SYSTEM_MATCHINGENGINE_H
#define TRADING_SYSTEM_MATCHINGENGINE_H

#pragma once
#include "MatchingEngineShard.h"
#include "../protocol/Protocol.h"
#include "../common/ThreadPool.h"

#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include <functional>

class MatchingEngine {
    // symbol → shard，每个交易对一个独立撮合引擎
    std::unordered_map<std::string, std::shared_ptr<MatchingEngineShard>> shards_;
    std::mutex shardMutex_;

    // 线程池（预留，用于后续扩展：如报盘、清算等）
    std::unique_ptr<ThreadPool> threadPool_;

public:
    MatchingEngine();
    ~MatchingEngine();

    /**
     * @brief 启动引擎（启动线程池）
     */
    void start();

    /**
     * @brief 停止引擎
     */
    void stop();

    /**
     * @brief 提交订单
     * @param symbol 交易对，如 "BTC/USD"
     * @param req 订单请求
     * @param cb 响应回调（由网络层提供）
     */
    void submit(const std::string& symbol,
                const OrderReq& req,
                std::function<void(OrderResp)> cb);

    /**
     * @brief 获取指定 symbol 的 shard（调试用）
     */
    std::shared_ptr<MatchingEngineShard> getShard(const std::string& symbol);

    /**
     * @brief 获取已创建的 shard 数量
     */
    size_t shardCount() const;
};

#endif //TRADING_SYSTEM_MATCHINGENGINE_H
