/**
  ******************************************************************************
  * @file           : main_client.cpp
  * @author         : vivi wu 
  * @brief          : 交易客户端（支持并发多线程）
  * @version        : 0.2.0
  * @date           : 30/04/26
  ******************************************************************************
  */

#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstring>
#include <vector>
#include <random>

#include "../src/protocol/Protocol.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888

class TradingClient {
private:
    int sockfd;
    struct sockaddr_in server_addr;
    std::atomic<bool> connected;

public:
    TradingClient() : connected(false) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            std::cerr << "Socket creation failed!" << std::endl;
            exit(EXIT_FAILURE);
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    }

    ~TradingClient() {
        if (connected) {
            close(sockfd);
        }
    }

    void connectToServer() {
        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Connection failed!" << std::endl;
            exit(EXIT_FAILURE);
        }
        connected = true;
        std::cout << "Connected to the server!" << std::endl;
    }

    void sendOrder(const OrderReq& orderReq) {
        int n = send(sockfd, &orderReq, orderReq.len, 0);
        if (n < 0) {
            std::cerr << "Send order failed!" << std::endl;
        }
    }

    bool receiveResponse(OrderResp& resp) {
        char buffer[256];
        int n = recv(sockfd, buffer, sizeof(buffer), 0);
        if (n >= static_cast<int>(sizeof(OrderResp))) {
            memcpy(&resp, buffer, sizeof(resp));
            return true;
        }
        return false;
    }

    void startTrading(int orderCount, const char* symbol) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> priceDist(900, 1100);
        std::uniform_int_distribution<> qtyDist(1, 100);

        for (int i = 0; i < orderCount; ++i) {
            OrderReq orderReq{};
            orderReq.len = sizeof(orderReq);
            orderReq.orderId = static_cast<uint64_t>(i + 1);
            orderReq.account = 1001;
            orderReq.price = priceDist(gen);
            orderReq.qty = qtyDist(gen);
            orderReq.side = (i % 2 == 0) ? 1 : 2;
            orderReq.ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            strncpy(orderReq.symbol, symbol, sizeof(orderReq.symbol) - 1);

            sendOrder(orderReq);

            OrderResp resp{};
            if (receiveResponse(resp)) {
                std::cout << "Order " << resp.orderId
                          << " status=" << static_cast<int>(resp.status)
                          << " filledQty=" << resp.filledQty
                          << " fillPrice=" << resp.fillPrice << std::endl;
            }
        }
    }
};

void workerThread(int threadId, int ordersPerThread, const char* symbol) {
    TradingClient client;
    client.connectToServer();
    std::cout << "[Thread " << threadId << "] Starting trading..." << std::endl;
    client.startTrading(ordersPerThread, symbol);
    std::cout << "[Thread " << threadId << "] Finished." << std::endl;
}

int main(int argc, char* argv[]) {
    int threads = 1;
    int ordersPerThread = 10;
    const char* symbol = "BTC/USD";

    if (argc > 1) threads = atoi(argv[1]);
    if (argc > 2) ordersPerThread = atoi(argv[2]);
    if (argc > 3) symbol = argv[3];

    std::cout << "Threads: " << threads
              << ", Orders per thread: " << ordersPerThread
              << ", Symbol: " << symbol << std::endl;

    std::vector<std::thread> workers;
    for (int i = 0; i < threads; ++i) {
        workers.emplace_back(workerThread, i, ordersPerThread, symbol);
    }

    for (auto& t : workers) {
        t.join();
    }

    std::cout << "All threads completed." << std::endl;
    return 0;
}
