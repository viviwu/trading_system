/**
  ******************************************************************************
  * @file           : bench_client.cpp
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */
// g++ -std=c++17 bench_client.cpp -o bench_client

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include <iostream>
#include <vector>
#include <cstring>
#include <chrono>

// ================= util =================

int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// ================= connection =================

struct Conn {
    int fd;
    bool connected{false};
    uint64_t send_ts{0};
};

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

    uint64_t totalReq = 0;
    uint64_t totalResp = 0;

    auto start = std::chrono::steady_clock::now();

    while (true) {
        int n = epoll_wait(epfd, events.data(), events.size(), 1000);

        for (int i = 0; i < n; ++i) {
            int idx = events[i].data.u32;
            auto& c = conns[idx];

            // ===== 连接建立 =====
            if (!c.connected && (events[i].events & EPOLLOUT)) {
                c.connected = true;
            }

            // ===== 写 =====
            if (events[i].events & EPOLLOUT) {
                const char* msg = "ping";

                int ret = write(c.fd, msg, strlen(msg));
                if (ret > 0) {
                    c.send_ts = std::chrono::steady_clock::now().time_since_epoch().count();
                    totalReq++;
                }
            }

            // ===== 读 =====
            if (events[i].events & EPOLLIN) {
                char buf[1024];

                while (true) {
                    int ret = read(c.fd, buf, sizeof(buf));

                    if (ret > 0) {
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

        // ===== 每秒打印 =====
        auto now = std::chrono::steady_clock::now();
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();

        if (sec >= 1) {
            std::cout << "QPS=" << totalResp << " conn=" << connCount << std::endl;

            totalReq = 0;
            totalResp = 0;
            start = now;
        }
    }
}