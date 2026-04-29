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

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(fd, (sockaddr*)&addr, sizeof(addr));

    while (true) {
        write(fd, "ping", 4);
    }
}