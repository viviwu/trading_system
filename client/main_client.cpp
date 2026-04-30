/**
  ******************************************************************************
  * @file           : main_client.cpp
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
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
#include <functional>

// 引入协议数据结构
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
        // 发送OrderReq数据包
        int len = orderReq.len;
        int n = send(sockfd, &orderReq, len, 0);
        if (n < 0) {
            std::cerr << "Send order failed!" << std::endl;
        }
    }

    void receiveResponse() {
        // 接收服务器响应数据
        char buffer[1024];
        int n = recv(sockfd, buffer, sizeof(buffer), 0);
        if (n > 0) {
            OrderResp* response = reinterpret_cast<OrderResp*>(buffer);
            std::cout << "Received Response:" << std::endl;
            std::cout << "Order ID: " << response->orderId << std::endl;
            std::cout << "Status: " << static_cast<int>(response->status) << std::endl;
            std::cout << "Filled Qty: " << response->filledQty << std::endl;
            std::cout << "Fill Price: " << response->fillPrice << std::endl;
        }
    }

    void startTrading(int orderCount) {
        for (int i = 0; i < orderCount; ++i) {
            OrderReq orderReq;
            orderReq.len = sizeof(orderReq);
            orderReq.orderId = static_cast<uint64_t>(i + 1);
            orderReq.account = 1001;  // 示例账户号
            orderReq.price = 1000 + i;  // 设置不同的价格
            orderReq.qty = 10;  // 每次下单数量
            orderReq.side = (i % 2 == 0) ? 1 : 2;  // 买（1）或卖（2）
            orderReq.ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();

            sendOrder(orderReq);
            receiveResponse();

            // 模拟交易延迟
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

int main() {
    TradingClient client;
    client.connectToServer();
    client.startTrading(10); // 启动10笔订单的交易模拟
    return 0;
}

/**
说明
连接服务器：
客户端通过 connectToServer() 方法建立 TCP 连接。
构建订单：
每个订单的数据结构 OrderReq 根据协议填充，包括 orderId、account、price、qty、side（买/卖方向）、ts（时间戳）等字段。
每次创建一个新的 OrderReq 结构体，模拟不同的订单请求。
发送订单：
使用 send() 系统调用发送订单请求。
接收响应：
客户端使用 recv() 接收服务器响应的 OrderResp 数据包，并打印响应中的订单 ID、状态、成交数量和成交价格等信息。
并发模拟：
为了模拟实际交易流量，客户端每隔 100 毫秒提交一笔订单，并接收服务器响应。

主要功能和扩展
创建多个订单：模拟真实的订单流量，且每个订单的价格、数量、方向都能有所变化。
多线程支持：你可以在 startTrading() 方法中加入多线程来模拟多个客户端并发提交订单。
响应解析：客户端接收服务器响应后，可以根据不同的订单状态（例如：已成交、未成交、部分成交）进行相应的处理。

下一步
多线程压测：将客户端扩展为多线程并发提交订单，模拟更高频的交易请求。
异常处理和回调机制：增强客户端对服务器异常和订单状态的处理能力，加入更精细的回调机制。

这个版本已经能够模拟基本的订单提交与响应，后续可以根据实际需要进行并发压测、网络延迟优化等。
*/
