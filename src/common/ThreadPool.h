/**
  ******************************************************************************
  * @file           : ThreadPool.h
  * @author         : vivi wu 
  * @brief          : 固定大小线程池
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */

#ifndef TRADING_SYSTEM_THREADPOOL_H
#define TRADING_SYSTEM_THREADPOOL_H

#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

/**
 * @brief 固定大小线程池
 * 
 * 用于 MatchingEngine 调度各 symbol 的撮合任务。
 * 线程数建议与 CPU 核心数一致或略多。
 */
class ThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};

public:
    explicit ThreadPool(size_t threads) {
        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mtx_);
                        cv_.wait(lock, [this] {
                            return stop_.load() || !tasks_.empty();
                        });
                        if (stop_.load() && tasks_.empty()) {
                            return;
                        }
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        stop_.store(true);
        cv_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief 提交任务到线程池
     */
    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            tasks_.push(std::move(task));
        }
        cv_.notify_one();
    }

    size_t size() const {
        return workers_.size();
    }
};

#endif // TRADING_SYSTEM_THREADPOOL_H
