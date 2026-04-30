# Trading System — 高性能证券交易系统雏形

> **定位**：一个基于 `epoll + Reactor` 的高性能、低延迟网关，结合 **lock-free + 多线程分片** 撮合引擎的证券交易系统雏形。
>
> **目标**：在单机上实现微秒级（us）延迟、十万级 QPS 的订单处理能力，并具备向分布式架构演进的扩展性。

---

## 一、项目概述

本项目是一个面向证券/加密货币交易场景的高性能撮合系统雏形，核心关注点：

- **低延迟**：从网关接收订单到返回撮合结果，追求微秒级（us）延迟
- **高吞吐**：通过 `无锁队列 + 多线程分片` 实现多交易对的并行撮合
- **模块化**：网络层、协议层、撮合层、风控层清晰解耦，便于独立迭代和替换
- **可扩展**：架构预留了线程池、分布式分片等扩展点，支持后续水平扩展

当前版本已实现：
- ✅ `epoll + Reactor` 模式的高并发 TCP 网关
- ✅ 二进制协议（`OrderReq / OrderResp`）
- ✅ **无锁环形队列**（`LockFreeQueue`）订单缓冲
- ✅ **按 Symbol 分片**的并行撮合引擎（`MatchingEngineShard`）
- ✅ **风控管理**（`RiskManager`：余额/持仓检查）
- ✅ 订单簿撮合（价格优先、时间优先）
- ✅ QPS / RTT 压测工具

---

## 二、技术架构

```
┌─────────────────────────────────────────────────────────────┐
│                        Client Layer                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │ main_client  │  │ bench_client │  │ bench_client_rtt │  │
│  └──────────────┘  └──────────────┘  └──────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │ TCP
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      Network Layer                           │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────┐  │
│  │ TcpServer│───▶│EventLoop │───▶│ Channel  │───▶│Buffer│  │
│  │(epoll LT)│    │(epoll_wait)│   │(EPOLLET) │    │      │  │
│  └──────────┘    └──────────┘    └──────────┘    └──────┘  │
│         │                                                    │
│         ▼                                                    │
│  ┌────────────────────────────────────────────────────┐     │
│  │ TcpConnection（解析 OrderReq，提取 symbol）          │     │
│  └────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Matching Engine                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │           MatchingEngine（全局管理）                   │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │   │
│  │  │Shard:BTC/USD│  │Shard:ETH/USD│  │Shard:...    │  │   │
│  │  │ ┌─────────┐ │  │ ┌─────────┐ │  │ ┌─────────┐ │  │   │
│  │  │ │LockFree │ │  │ │LockFree │ │  │ │LockFree │ │  │   │
│  │  │ │ Queue   │ │  │ │ Queue   │ │  │ │ Queue   │ │  │   │
│  │  │ └────┬────┘ │  │ └────┬────┘ │  │ └────┬────┘ │  │   │
│  │  │      ▼      │  │      ▼      │  │      ▼      │  │   │
│  │  │ ┌─────────┐ │  │ ┌─────────┐ │  │ ┌─────────┐ │  │   │
│  │  │ │  Worker │ │  │ │  Worker │ │  │ │  Worker │ │  │   │
│  │  │ │ Thread  │ │  │ │ Thread  │ │  │ │ Thread  │ │  │   │
│  │  │ └────┬────┘ │  │ └────┬────┘ │  │ └────┬────┘ │  │   │
│  │  │      ▼      │  │      ▼      │  │      ▼      │  │   │
│  │  │ ┌─────────┐ │  │ ┌─────────┐ │  │ ┌─────────┐ │  │   │
│  │  │ │RiskMgr  │ │  │ │RiskMgr  │ │  │ │RiskMgr  │ │  │   │
│  │  │ └────┬────┘ │  │ └────┬────┘ │  │ └────┬────┘ │  │   │
│  │  │      ▼      │  │      ▼      │  │      ▼      │  │   │
│  │  │ ┌─────────┐ │  │ ┌─────────┐ │  │ ┌─────────┐ │  │   │
│  │  │ │OrderBook│ │  │ │OrderBook│ │  │ │OrderBook│ │  │   │
│  │  │ └─────────┘ │  │ └─────────┘ │  │ └─────────┘ │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 2.1 分层说明

| 层级 | 模块 | 职责 |
|------|------|------|
| **Client** | `main_client` / `bench_client` / `bench_client_rtt` | 模拟订单发送、QPS 压测、RTT 延迟统计 |
| **Network** | `TcpServer` / `TcpConnection` / `EventLoop` / `Channel` / `Buffer` | 基于 epoll 的高并发 TCP 网关，支持 ET 模式 |
| **Protocol** | `OrderReq` / `OrderResp` | 紧凑二进制协议，`#pragma pack(push,1)` 对齐 |
| **Engine** | `MatchingEngine` / `MatchingEngineShard` / `OrderBook` / `RiskManager` | 多 symbol 分片撮合、无锁队列、风控检查 |
| **Common** | `LockFreeQueue` / `ThreadPool` | 无锁环形队列、线程池 |

---

## 三、核心特点

### 3.1 Lock-free 无锁队列

- 采用 **SPSC（单生产者单消费者）** 环形缓冲区设计
- 基于 `std::atomic<size_t>` 的 head/tail 指针 + `memory_order` 内存屏障
- 网络线程（生产者）`enqueue`，撮合线程（消费者）`dequeue`，**零锁竞争**
- 支持非平凡类型（如含 `std::function` 回调的 `OrderTask`）

### 3.2 Symbol 分片（Sharding）

- 每个交易对（如 `BTC/USD`、`ETH/USD`）对应一个独立的 `MatchingEngineShard`
- 每个 Shard 包含：**独立订单簿 + 无锁队列 + 风控器 + 专属线程**
- **不同 symbol 完全并行**，天然水平扩展；同一 symbol 串行撮合，保证时序一致性
- Shard **懒创建**：首次收到某 symbol 订单时自动初始化，无需预配置

### 3.3 风控前置（RiskManager）

- 每个 Shard 内部集成 `RiskManager`，利用单线程特性 **无需加锁**
- 支持：
  - **余额检查**：买单时检查 `balance >= price * qty`
  - **持仓检查**：卖单时检查 `position >= qty`
  - **冻结/释放**：撮合前冻结，部分成交后释放未成交部分

### 3.4 订单簿撮合（价格优先 + 时间优先）

- **买方（Bids）**：`std::map<double, std::deque<Order>, std::greater<>>`，价格从高到低
- **卖方（Asks）**：`std::map<double, std::deque<Order>>`，价格从低到高
- 撮合时返回 `std::vector<Fill>` 成交明细，支持计算**成交均价**

### 3.5 完整的压测工具链

| 工具 | 用途 | 关键指标 |
|------|------|----------|
| `main_client` | 功能验证、多线程并发测试 | 订单状态、成交明细 |
| `bench_client` | QPS 吞吐量压测 | 每秒请求/响应数 |
| `bench_client_rtt` | RTT 延迟压测 | p50 / p99 / p999 / max |

---

## 四、核心实现

### 4.1 协议定义 (`src/protocol/Protocol.h`)

```cpp
#pragma pack(push,1)

struct OrderReq {
    uint32_t len;        // 包总长度
    uint64_t orderId;    // 订单ID
    uint32_t account;    // 账户ID
    double   price;      // 委托价格
    uint32_t qty;        // 委托数量
    uint8_t  side;       // 1=Buy, 2=Sell
    uint64_t ts;         // 客户端时间戳
    char     symbol[8];  // 交易对，如 "BTC/USD"
};

struct OrderResp {
    uint32_t len;        // 包总长度
    uint64_t orderId;    // 订单ID
    uint8_t  status;     // 0=全部成交, 1=部分成交, 2=风控拒绝, 3=已挂单
    uint32_t filledQty;  // 已成交数量
    double   fillPrice;  // 成交均价
    uint64_t ts;         // 服务端时间戳
};

#pragma pack(pop)
```

### 4.2 无锁队列 (`src/common/LockFreeQueue.h`)

```cpp
template <typename T>
class LockFreeQueue {
    struct Slot {
        std::atomic<bool> ready{false};
        T data{};
        // 自定义 move 语义，支持 std::atomic + 非平凡类型
        Slot(Slot&& other) noexcept
            : ready(other.ready.exchange(false)), data(std::move(other.data)) {}
    };
    std::vector<Slot> buffer_;
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
    // ...
};
```

### 4.3 撮合分片 (`src/engine/MatchingEngineShard.cpp`)

每个 Shard 的独立线程循环：

```cpp
void MatchingEngineShard::processOrders() {
    OrderTask task;
    while (queue_.dequeue(task)) {
        // 1. 风控检查
        if (!riskManager_.check(task.req)) {
            // 返回拒绝状态...
            continue;
        }
        // 2. 冻结资金/持仓
        riskManager_.apply(task.req);
        // 3. 撮合
        Order ord{...};
        auto fills = orderBook_->match(ord);
        // 4. 释放未成交部分 & 构造响应
        riskManager_.release(task.req, filledQty);
        task.callback(resp);
    }
}
```

### 4.4 全局引擎 (`src/engine/MatchingEngine.cpp`)

```cpp
void MatchingEngine::submit(const std::string& symbol,
                            const OrderReq& req,
                            std::function<void(OrderResp)> cb) {
    std::shared_ptr<MatchingEngineShard> shard;
    {
        std::lock_guard<std::mutex> lk(shardMutex_);
        // 懒创建 shard
        if (shards_.find(symbol) == shards_.end()) {
            shard = std::make_shared<MatchingEngineShard>(65536);
            shard->start();
            shards_[symbol] = shard;
        } else {
            shard = shards_[symbol];
        }
    }
    // 入队（无锁，零竞争）
    shard->submitOrder(req, std::move(cb));
}
```

---

## 五、项目结构

```
trading_system/
├── CMakeLists.txt
├── README.md
├── client/
│   ├── main_client.cpp        # 基础交易客户端（多线程）
│   ├── bench_client.cpp       # QPS 压测客户端
│   └── bench_client_rtt.cpp   # RTT 延迟压测客户端
└── src/
    ├── main_server.cpp        # 服务端入口
    ├── protocol/
    │   └── Protocol.h         # 二进制协议定义
    ├── common/
    │   ├── LockFreeQueue.h    # 无锁环形队列
    │   ├── ThreadPool.h       # 线程池
    │   └── NonCopyable.h      # 非拷贝基类（预留）
    ├── net/
    │   ├── TcpServer.h/cpp    # TCP 服务端（epoll）
    │   ├── TcpConnection.h/cpp# TCP 连接管理
    │   ├── EventLoop.h/cpp    # epoll 事件循环
    │   ├── Channel.h/cpp      # IO 事件通道
    │   └── Buffer.h/cpp       # 应用层缓冲区
    └── engine/
        ├── MatchingEngine.h/cpp      # 全局撮合引擎（分片管理）
        ├── MatchingEngineShard.h/cpp # 单 symbol 撮合分片
        ├── OrderBook.h/cpp           # 订单簿撮合逻辑
        └── RiskManager.h             # 风控管理
```

---

## 六、快速开始

### 6.1 编译

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

编译产物位于 `bin/` 目录：
- `server` — 服务端
- `client` — 基础客户端
- `bench_client` — QPS 压测客户端
- `bench_client_rtt` — RTT 延迟压测客户端

### 6.2 运行服务端

```bash
./bin/server
# 监听 0.0.0.0:8888
```

### 6.3 运行客户端

```bash
# 基础客户端：2 线程，每线程 5 笔订单，交易对 BTC/USD
./bin/client 2 5 BTC/USD

# QPS 压测：4 条连接
./bin/bench_client 127.0.0.1 8888 4 BTC/USD

# RTT 延迟压测：4 条连接
./bin/bench_client_rtt 127.0.0.1 8888 4 BTC/USD
```

---

## 七、性能基准（参考值）

在开发机上的初步测试结果（单机上同机测试，含网络栈开销）：

| 指标 | 数值 | 说明 |
|------|------|------|
| **QPS** | ~38,000 req/s | 4 连接持续压测 |
| **架构并发能力** | Symbol 级并行 | 每新增一个 symbol 增加一个独立线程 |
| **延迟瓶颈** | 网络栈 + 序列化 | 同机测试时主要为内核网络栈开销 |

> **注**：当前为雏形阶段，撮合逻辑为模拟实现，未接入真实交易所行情。实际生产环境的延迟取决于网络拓扑、内核调优（如 `busy poll`、`SO_BUSY_POLL`）、以及是否使用 DPDK/kernel-bypass 技术。

---

## 八、后续迭代优化建议

### 8.1 性能优化（近期）

1. **CPU 亲和性绑定（CPU Pinning）**
   - 将每个 Shard 的撮合线程绑定到独立 CPU 核心，避免上下文切换和缓存失效
   - 使用 `pthread_setaffinity_np` 实现

2. **忙等优化（Busy Spin / Hybrid Polling）**
   - 当前 Shard 线程在队列空时 `sleep_for(10us)`，对于低延迟场景可改为：
     - 短周期忙等（如 `PAUSE` 指令自旋 1000 次）
     - 或结合 `eventfd` / `futex` 实现事件通知，兼顾延迟和 CPU 占用

3. **内存池与对象复用**
   - `OrderTask` 和 `Fill` 的频繁分配可使用固定大小内存池（如 `boost::pool` 或自研 `SlabAllocator`）
   - 减少 `new/delete` 带来的系统调用和页表抖动

4. **Zero-Copy 网络收发**
   - 当前 `Buffer` 使用 `std::string` 做底层存储，存在拷贝
   - 可替换为基于 `io_uring` 的零拷贝方案，或使用环形缓冲区 + `sendmsg/recvmsg`

5. **协议序列化优化**
   - 当前使用 `memcpy` 序列化结构体，可考虑 `FlatBuffers` / `Cap'n Proto` 或自研紧凑格式
   - 支持批量订单（Batch Order）减少系统调用次数

### 8.2 功能扩展（中期）

6. **订单类型扩展**
   - 支持 `LIMIT` / `MARKET` / `STOP-LIMIT` / `IOC` / `FOK` 等订单类型
   - 部分成交（Partial Fill）的精细状态管理

7. **持久化与回放**
   - 引入 WAL（Write-Ahead Log）记录订单和成交流水
   - 支持系统重启后的订单簿状态恢复（Snapshot + Log Replay）

8. **多账户与权限体系**
   - `RiskManager` 扩展为支持多币种保证金、杠杆、强平逻辑
   - 接入真实账户系统（如数据库或缓存）

9. **行情发布（Market Data）**
   - 撮合完成后主动推送 `Trade Tick` / `OrderBook L2` 到行情服务器
   - 引入发布-订阅模型（如 `mmap` 共享内存环形缓冲区）

### 8.3 架构演进（远期）

10. **分布式撮合**
    - 将 `MatchingEngineShard` 分布到多台机器，通过 **RDMA / DPDK** 或高性能消息队列（如 ` Aeron`）通信
    - 引入一致性哈希进行 symbol 路由

11. **FPGA/硬件加速**
    - 将订单簿撮合逻辑下沉到 FPGA，实现亚微秒（sub-μs）级别延迟
    - 网关层使用 `kernel bypass`（如 `DPDK`、`Solarflare OpenOnload`）

12. **微服务化**
    - 网关、撮合、风控、清算分离为独立服务
    - 使用 `gRPC` 或自研基于共享内存的 IPC 进行服务间通信

---

## 九、License

MIT License

---

> **作者**：vivi wu  
> **版本**：0.2.0  
> **日期**：2026-04-30
