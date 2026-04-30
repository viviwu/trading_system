/**
  ******************************************************************************
  * @file           : MatchingEngine.cpp
  * @author         : vivi wu 
  * @brief          : 全局撮合引擎实现
  * @version        : 0.2.0
  * @date           : 30/04/26
  ******************************************************************************
  */

#include "MatchingEngine.h"
#include <thread>

MatchingEngine::MatchingEngine() {
    // 线程池大小默认为 CPU 核心数，预留用于后续扩展任务
    unsigned int n = std::thread::hardware_concurrency();
    if (n == 0) n = 4;
    threadPool_ = std::make_unique<ThreadPool>(n);
}

MatchingEngine::~MatchingEngine() {
    stop();
}

void MatchingEngine::start() {
    // 线程池已在构造时启动
    // shard 的线程在首次 submit 时懒启动
}

void MatchingEngine::stop() {
    std::lock_guard<std::mutex> lk(shardMutex_);
    for (auto& entry : shards_) {
        if (entry.second) {
            entry.second->stop();
        }
    }
}

void MatchingEngine::submit(const std::string& symbol,
                            const OrderReq& req,
                            std::function<void(OrderResp)> cb) {
    std::shared_ptr<MatchingEngineShard> shard;
    {
        std::lock_guard<std::mutex> lk(shardMutex_);
        auto it = shards_.find(symbol);
        if (it == shards_.end()) {
            shard = std::make_shared<MatchingEngineShard>(65536);
            shard->start();
            shards_[symbol] = shard;
        } else {
            shard = it->second;
        }
    }
    shard->submitOrder(req, std::move(cb));
}

std::shared_ptr<MatchingEngineShard> MatchingEngine::getShard(
    const std::string& symbol) {
    std::lock_guard<std::mutex> lk(shardMutex_);
    auto it = shards_.find(symbol);
    if (it != shards_.end()) {
        return it->second;
    }
    return nullptr;
}

size_t MatchingEngine::shardCount() const {
    std::lock_guard<std::mutex> lk(const_cast<std::mutex&>(shardMutex_));
    return shards_.size();
}
