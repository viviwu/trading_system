[//]: # (Order + Risk)

```
struct Account {
double balance{1e9};
uint32_t position{100000};
};

class RiskManager {
std::unordered_map<uint32_t, Account> accounts;

public:
bool check(const OrderReq& ord) {
auto& acc = accounts[ord.account];

        if (ord.side == 1) { // buy
            return acc.balance >= ord.price * ord.qty;
        } else { // sell
            return acc.position >= ord.qty;
        }
    }

    void apply(const OrderReq& ord) {
        auto& acc = accounts[ord.account];

        if (ord.side == 1) {
            acc.balance -= ord.price * ord.qty;
        } else {
            acc.position -= ord.qty;
        }
    }
};
```


# TODO:把 MatchingEngine 升级为 lock-free + 多线程 shard（真正交易级）

```asm
升级 MatchingEngine 为 lock-free + 多线程分片架构，意味着我们需要解决并发性能瓶颈，并引入分片来实现每个 symbol的并行处理，同时尽量避免锁竞争。这个过程包括：

Lock-free 队列：使用 无锁队列来处理订单，避免传统的队列在高并发下的锁竞争。
分片：每个交易对或市场（symbol）独立一个撮合线程（或者任务池），这可以显著提升系统的吞吐量。
缓存友好：减少锁争用和缓存错乱，提升性能。
关键变化：
无锁队列：采用 std::atomic 或 环形缓冲区来实现无锁操作。
多线程分片：每个 symbol（交易对）分配一个独立的撮合引擎实例，分布式处理订单，避免瓶颈。
线程池：任务投递到线程池中，执行撮合操作。
锁优化：减少冗余锁，尽量靠 CAS (Compare And Swap) 和无锁设计来提升并发性能。
```