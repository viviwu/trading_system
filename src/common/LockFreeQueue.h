/**
  ******************************************************************************
  * @file           : LockFreeQueue.h
  * @author         : vivi wu 
  * @brief          : 无锁环形队列（Single-Producer Single-Consumer）
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */

#ifndef TRADING_SYSTEM_LOCKFREEQUEUE_H
#define TRADING_SYSTEM_LOCKFREEQUEUE_H

#pragma once
#include <atomic>
#include <vector>
#include <memory>
#include <new>

/**
 * @brief 基于环形缓冲区的无锁队列
 * 
 * 设计为 SPSC（单生产者单消费者）场景：
 * - 生产者：网络线程 enqueue
 * - 消费者：撮合线程 dequeue
 * 
 * 使用 std::atomic 和 memory_order 保证线程安全。
 */
template <typename T>
class LockFreeQueue {
private:
    struct Slot {
        std::atomic<bool> ready{false};
        T data{};

        Slot() = default;

        Slot(Slot&& other) noexcept
            : ready(other.ready.exchange(false)), data(std::move(other.data)) {}

        Slot& operator=(Slot&& other) noexcept {
            ready.store(other.ready.exchange(false));
            data = std::move(other.data);
            return *this;
        }

        // 禁止拷贝
        Slot(const Slot&) = delete;
        Slot& operator=(const Slot&) = delete;
    };

    std::vector<Slot> buffer_;
    const size_t capacity_;
    alignas(64) std::atomic<size_t> head_{0};  // 读指针
    alignas(64) std::atomic<size_t> tail_{0};  // 写指针

public:
    explicit LockFreeQueue(size_t cap)
        : capacity_(cap) {
        buffer_.resize(capacity_);
    }

    ~LockFreeQueue() = default;

    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;

    /**
     * @brief 入队
     * @return true 成功, false 队列满
     */
    bool enqueue(const T& item) {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        const size_t next = (tail + 1) % capacity_;

        // 检查队列是否已满
        if (next == head_.load(std::memory_order_acquire)) {
            return false;
        }

        // 写入数据并标记就绪
        buffer_[tail].data = item;
        buffer_[tail].ready.store(true, std::memory_order_release);
        tail_.store(next, std::memory_order_release);
        return true;
    }

    /**
     * @brief 出队
     * @return true 成功, false 队列空
     */
    bool dequeue(T& item) {
        const size_t head = head_.load(std::memory_order_relaxed);

        // 检查队列是否为空
        if (head == tail_.load(std::memory_order_acquire)) {
            return false;
        }

        // 检查数据是否已写入完成（防止指令重排导致的脏读）
        if (!buffer_[head].ready.load(std::memory_order_acquire)) {
            return false;
        }

        item = buffer_[head].data;
        buffer_[head].ready.store(false, std::memory_order_release);
        head_.store((head + 1) % capacity_, std::memory_order_release);
        return true;
    }

    size_t capacity() const { return capacity_; }

    bool empty() const {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }
};

#endif // TRADING_SYSTEM_LOCKFREEQUEUE_H
