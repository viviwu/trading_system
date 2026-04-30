升级 `MatchingEngine` 为 **lock-free + 多线程分片**架构，意味着我们需要解决**并发性能瓶颈**，并引入**分片**来实现**每个 symbol**的并行处理，同时尽量避免锁竞争。这个过程包括：

1. **Lock-free 队列**：使用 **无锁队列**来处理订单，避免传统的队列在高并发下的锁竞争。
2. **分片**：每个交易对或市场（symbol）独立一个撮合线程（或者任务池），这可以显著提升系统的吞吐量。
3. **缓存友好**：减少锁争用和缓存错乱，提升性能。

### 关键变化：

* **无锁队列**：采用 **`std::atomic`** 或 **环形缓冲区**来实现无锁操作。
* **多线程分片**：每个 symbol（交易对）分配一个独立的撮合引擎实例，分布式处理订单，避免瓶颈。
* **线程池**：任务投递到线程池中，执行撮合操作。
* **锁优化**：减少冗余锁，尽量靠 **CAS (Compare And Swap)** 和**无锁设计**来提升并发性能。

---

# 一、分片设计（Symbol Sharding）

我们首先为每个 `symbol`（例如 BTC/USD, ETH/USD）创建一个 **单独的 OrderBook + MatchingEngine**。

### 数据结构：

```cpp
// 每个交易对（symbol）对应一个 Shard（撮合引擎实例）
std::unordered_map<std::string, std::shared_ptr<MatchingEngineShard>> shards;
```

每个 `MatchingEngineShard` 包含：

* 一个独立的 `OrderBook` 进行撮合。
* 一个 **无锁队列**用于接收订单。
* 一个 **撮合线程** 来执行订单撮合。

---

# 二、无锁队列设计

我们使用 **无锁队列**来替代传统的 `std::queue` 和 `std::mutex`，常见的设计是使用环形缓冲区（**lock-free ring buffer**）。下面给出一个简单的实现框架：

```cpp
template <typename T>
class LockFreeQueue {
private:
    std::vector<std::atomic<T>> buffer;
    std::atomic<size_t> head, tail;
    size_t capacity;

public:
    LockFreeQueue(size_t cap) : capacity(cap), head(0), tail(0) {
        buffer.resize(capacity);
    }

    bool enqueue(const T& item) {
        size_t tail_pos = tail.load();
        if ((tail_pos + 1) % capacity == head.load()) {
            return false; // 队列满了
        }
        buffer[tail_pos].store(item);
        tail.store((tail_pos + 1) % capacity);
        return true;
    }

    bool dequeue(T& item) {
        size_t head_pos = head.load();
        if (head_pos == tail.load()) {
            return false; // 队列空了
        }
        item = buffer[head_pos].load();
        head.store((head_pos + 1) % capacity);
        return true;
    }
};
```

`LockFreeQueue` 的核心操作就是 `enqueue` 和 `dequeue`，通过 **原子操作** (`std::atomic`) 来保证线程安全。

---

# 三、无锁撮合引擎和分片（Sharding）

### 每个 `symbol` 都有独立的 `MatchingEngineShard` 和 `OrderBook`

```cpp
class MatchingEngineShard {
public:
    std::shared_ptr<OrderBook> orderBook;
    LockFreeQueue<Order> orderQueue;

    MatchingEngineShard(size_t capacity) 
        : orderQueue(capacity) {
        orderBook = std::make_shared<OrderBook>();
    }

    void processOrders() {
        Order order;
        while (orderQueue.dequeue(order)) {
            orderBook->match(order);
        }
    }

    void submitOrder(const Order& order) {
        orderQueue.enqueue(order);
    }
};
```

### 启动多个撮合线程来处理订单：

```cpp
void MatchingEngine::startShards() {
    for (auto& shard_entry : shards) {
        // 每个 symbol 启动一个独立的撮合线程
        std::thread([shard = shard_entry.second]() {
            while (true) {
                shard->processOrders();
            }
        }).detach();
    }
}
```

---

# 四、更新 `MatchingEngine` 使用分片（Sharding）

### `MatchingEngine` 的更新：

```cpp
class MatchingEngine {
    std::unordered_map<std::string, std::shared_ptr<MatchingEngineShard>> shards;

public:
    void submitOrder(const std::string& symbol, const Order& order) {
        if (shards.find(symbol) == shards.end()) {
            shards[symbol] = std::make_shared<MatchingEngineShard>(10000); // 初始化每个symbol的shard
        }
        shards[symbol]->submitOrder(order);
    }

    void start() {
        for (auto& shard : shards) {
            shard.second->processOrders();
        }
    }
};
```

### 提交订单到具体的 `symbol`（例如：BTC/USD）：

```cpp
engine.submitOrder("BTC/USD", order);
```

---

# 五、线程池（用于撮合任务）

我们使用一个简单的 **线程池** 来处理撮合任务。每个 `symbol` 在启动时都会有一个专门的工作线程池。

```cpp
class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop;

public:
    ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        cv.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();
        for (auto& worker : workers) {
            worker.join();
        }
    }

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            tasks.push(std::move(task));
        }
        cv.notify_one();
    }
};
```

使用 `ThreadPool` 提交拍卖任务：

```cpp
class MatchingEngineShard {
    ThreadPool pool;

    // 其余代码保持不变
};
```

---

# 六、最终代码结构（简化版）

1. **Order**：订单结构体（包含订单信息）。
2. **OrderBook**：订单簿，用于每个交易对的订单撮合。
3. **LockFreeQueue**：无锁队列，保证多线程下的高效入队出队。
4. **MatchingEngineShard**：每个交易对的独立撮合引擎，包括无锁队列和撮合逻辑。
5. **MatchingEngine**：全局撮合引擎，包含多个 `MatchingEngineShard`，负责处理不同的交易对。
6. **ThreadPool**：线程池，分配给每个 `MatchingEngineShard` 进行任务执行。

---

# 七、性能优化和监控

1. **无锁队列性能**：对于高并发操作，`LockFreeQueue` 提供了高效的入队和出队操作，减少了传统锁带来的开销。
2. **分片**：通过分片技术，订单撮合操作会在多个线程上并行进行，减少了单线程的瓶颈。
3. **线程池**：使用线程池来控制撮合线程数量，避免每次订单提交时都创建新线程，提高了系统的吞吐量。

---

# 八、下一步工作

1. **测试**：将这个架构应用到一个多用户高并发场景中，进行性能测试（例如：每秒处理多少订单，p99延迟等）。
2. **分布式撮合**：如果需要，可以将多个 `MatchingEngineShard` 分布到多个机器上，通过消息队列或 RPC 进行通信，实现分布式撮合。
3. **进一步的优化**：可以通过 **CPU亲和性**（CPU pinning）来优化线程调度，确保线程在特定的CPU核心上运行。

---

你现在可以基于这个设计开始进行优化和测试，之后我们可以进一步扩展到更加复杂的分布式撮合引擎。
