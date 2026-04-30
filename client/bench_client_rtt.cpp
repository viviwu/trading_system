/**
  ******************************************************************************
  * @file           : BenchClient.h
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */

// g++ -std=c++17 bench_client_rtt.cpp -o bench_client

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#include <iostream>
#include <vector>
#include <cstring>
#include <chrono>
#include <algorithm>

using Clock = std::chrono::steady_clock;

// ================= util =================

int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

inline uint64_t now_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        Clock::now().time_since_epoch()).count();
}

// ================= connection =================

struct Conn {
    int fd;
    bool connected{false};
    uint64_t send_ts{0};
};

// ================= percentile =================

uint32_t percentile(std::vector<uint32_t>& v, double p) {
    if (v.empty()) return 0;
    size_t idx = (size_t)(p * v.size());
    if (idx >= v.size()) idx = v.size() - 1;

    std::nth_element(v.begin(), v.begin() + idx, v.end());
    return v[idx];
}

// ================= main =================

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: ./bench_client ip port connections\n";
        return 0;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int connCount = atoi(argv[3]);

    int epfd = epoll_create1(0);
    std::vector<Conn> conns(connCount);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    // ===== 建立连接 =====
    for (int i = 0; i < connCount; ++i) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        setNonBlocking(fd);

        connect(fd, (sockaddr*)&addr, sizeof(addr));

        epoll_event ev{};
        ev.data.u32 = i;
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;

        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

        conns[i].fd = fd;
    }

    std::vector<epoll_event> events(1024);
    std::vector<uint32_t> latencies;
    latencies.reserve(1'000'000);

    uint64_t totalResp = 0;

    auto start = Clock::now();

    while (true) {
        int n = epoll_wait(epfd, events.data(), events.size(), 100);

        for (int i = 0; i < n; ++i) {
            int idx = events[i].data.u32;
            auto& c = conns[idx];

            if (!c.connected && (events[i].events & EPOLLOUT)) {
                c.connected = true;
            }

            // ===== 写 =====
            if (events[i].events & EPOLLOUT) {
                const char* msg = "ping";

                int ret = write(c.fd, msg, 4);
                if (ret > 0) {
                    c.send_ts = now_us();
                }
            }

            // ===== 读 =====
            if (events[i].events & EPOLLIN) {
                char buf[1024];

                while (true) {
                    int ret = read(c.fd, buf, sizeof(buf));

                    if (ret > 0) {
                        uint64_t rtt = now_us() - c.send_ts;
                        latencies.push_back((uint32_t)rtt);
                        totalResp++;

                    } else if (ret == 0) {
                        close(c.fd);
                        break;
                    } else {
                        if (errno == EAGAIN) break;
                        break;
                    }
                }
            }
        }

        // ===== 每秒统计 =====
        auto now = Clock::now();
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();

        if (sec >= 1 && !latencies.empty()) {

            auto tmp = latencies; // 拷贝一份做统计

            uint32_t p50  = percentile(tmp, 0.50);
            uint32_t p90  = percentile(tmp, 0.90);
            uint32_t p99  = percentile(tmp, 0.99);
            uint32_t p999 = percentile(tmp, 0.999);
            uint32_t max  = *std::max_element(tmp.begin(), tmp.end());

            std::cout << "QPS=" << totalResp
                      << " p50=" << p50 << "us"
                      << " p99=" << p99 << "us"
                      << " p999=" << p999 << "us"
                      << " max=" << max << "us"
                      << std::endl;

            latencies.clear();
            totalResp = 0;
            start = now;
        }
    }
}