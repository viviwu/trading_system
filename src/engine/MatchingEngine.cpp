/**
  ******************************************************************************
  * @file           : MatchingEngine.cpp
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#include "MatchingEngine.h"
#include <thread>

void MatchingEngine::submit(const OrderReq& req,
                            std::function<void(OrderResp)> cb) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        q_.emplace(req, cb);
    }
    cv_.notify_one();
}

void MatchingEngine::start() {
    std::thread([this] {
        while (true) {
            std::unique_lock<std::mutex> lk(mtx_);
            cv_.wait(lk, [&]{ return !q_.empty(); });

            auto [req, cb] = q_.front();
            q_.pop();
            lk.unlock();

            Order ord{req.orderId, req.price, req.qty, req.side};
            book_.match(ord);

            OrderResp resp{};
            resp.len = sizeof(resp);
            resp.orderId = req.orderId;
            resp.status = 0;

            cb(resp);
        }
    }).detach();
}