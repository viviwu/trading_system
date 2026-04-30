/**
  ******************************************************************************
  * @file           : MatchingEngineShard.cpp
  * @author         : vivi wu 
  * @brief          : 撮合分片实现
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */

#include "MatchingEngineShard.h"
#include <chrono>

MatchingEngineShard::MatchingEngineShard(size_t queueCapacity)
    : queue_(queueCapacity) {
    orderBook_ = std::make_unique<OrderBook>();
}

MatchingEngineShard::~MatchingEngineShard() {
    stop();
}

bool MatchingEngineShard::submitOrder(const OrderReq& req,
                                       std::function<void(OrderResp)> cb) {
    OrderTask task{req, std::move(cb)};
    return queue_.enqueue(std::move(task));
}

void MatchingEngineShard::start() {
    running_.store(true);
    worker_ = std::thread([this] {
        while (running_.load()) {
            processOrders();
            // 队列空时短暂休眠，避免忙等（可用 eventfd 优化，雏形阶段用 usleep）
            if (queue_.empty()) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
    });
}

void MatchingEngineShard::stop() {
    running_.store(false);
    if (worker_.joinable()) {
        worker_.join();
    }
}

void MatchingEngineShard::processOrders() {
    OrderTask task;
    while (queue_.dequeue(task)) {
        const OrderReq& req = task.req;
        OrderResp resp{};
        resp.len = sizeof(resp);
        resp.orderId = req.orderId;
        resp.ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();

        // 风控检查
        if (!riskManager_.check(req)) {
            resp.status = 2;  // 风控拒绝
            resp.filledQty = 0;
            resp.fillPrice = 0.0;
            if (task.callback) {
                task.callback(resp);
            }
            continue;
        }

        // 应用风控（冻结资金/持仓）
        riskManager_.apply(req);

        // 撮合
        Order ord{req.orderId, req.price, req.qty, req.side};
        auto fills = orderBook_->match(ord);

        // 计算成交情况
        uint32_t totalFilled = req.qty - ord.qty;  // 原始数量 - 剩余数量 = 已成交
        double avgPrice = 0.0;
        if (!fills.empty()) {
            double totalValue = 0.0;
            for (const auto& f : fills) {
                totalValue += f.price * f.qty;
            }
            avgPrice = totalValue / totalFilled;
        }

        // 释放未成交部分
        if (ord.qty > 0) {
            riskManager_.release(req, totalFilled);
            resp.status = (totalFilled > 0) ? 1 : 3;  // 部分成交 或 未成交（已挂单）
        } else {
            resp.status = 0;  // 全部成交
        }

        resp.filledQty = totalFilled;
        resp.fillPrice = avgPrice;

        if (task.callback) {
            task.callback(resp);
        }
    }
}
