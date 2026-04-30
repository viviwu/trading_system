/**
  ******************************************************************************
  * @file           : MatchingEngineShard.h
  * @author         : vivi wu 
  * @brief          : 单个 symbol 的撮合引擎分片
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */

#ifndef TRADING_SYSTEM_MATCHINGENGINESHARD_H
#define TRADING_SYSTEM_MATCHINGENGINESHARD_H

#pragma once
#include "OrderBook.h"
#include "RiskManager.h"
#include "../protocol/Protocol.h"
#include "../common/LockFreeQueue.h"

#include <atomic>
#include <thread>
#include <functional>

/**
 * @brief 订单任务，包含请求和回调
 */
struct OrderTask {
    OrderReq req;
    std::function<void(OrderResp)> callback;
};

/**
 * @brief 单个 symbol 的撮合引擎分片
 * 
 * 每个 shard 拥有：
 * - 独立的 OrderBook（线程安全，因为只有一个线程访问）
 * - 独立的 RiskManager（同上）
 * - 无锁队列 LockFreeQueue（网络线程 enqueue，撮合线程 dequeue）
 * - 独立的撮合线程
 */
class MatchingEngineShard {
public:
    explicit MatchingEngineShard(size_t queueCapacity = 65536);
    ~MatchingEngineShard();

    // 禁止拷贝
    MatchingEngineShard(const MatchingEngineShard&) = delete;
    MatchingEngineShard& operator=(const MatchingEngineShard&) = delete;

    /**
     * @brief 提交订单到无锁队列（由网络线程调用）
     * @return true 入队成功, false 队列满
     */
    bool submitOrder(const OrderReq& req, std::function<void(OrderResp)> cb);

    /**
     * @brief 启动撮合线程
     */
    void start();

    /**
     * @brief 停止撮合线程
     */
    void stop();

    /**
     * @brief 处理队列中的一批订单（由撮合线程调用）
     */
    void processOrders();

private:
    std::unique_ptr<OrderBook> orderBook_;
    LockFreeQueue<OrderTask> queue_;
    RiskManager riskManager_;

    std::atomic<bool> running_{false};
    std::thread worker_;
};

#endif // TRADING_SYSTEM_MATCHINGENGINESHARD_H
