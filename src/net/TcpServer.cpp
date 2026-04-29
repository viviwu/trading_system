/**
  ******************************************************************************
  * @file           : TcpServer.cpp
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#include "TcpServer.h"
#include "EventLoop.h"
#include "Channel.h"
#include "TcpConnection.h"
#include "../engine/MatchingEngine.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

static int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

TcpServer::TcpServer(int port, MatchingEngine* engine)
    : engine_(engine) {

    loop_ = new EventLoop();

    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listenfd_, (sockaddr*)&addr, sizeof(addr));
    listen(listenfd_, 1024);

    setNonBlocking(listenfd_);

    auto ch = new Channel(loop_, listenfd_);
    ch->setReadCallback([this]() { handleAccept(); });
    ch->enableRead();
}

void TcpServer::handleAccept() {
    while (true) {
        sockaddr_in client{};
        socklen_t len = sizeof(client);

        int fd = accept(listenfd_, (sockaddr*)&client, &len);

        if (fd < 0) break;

        setNonBlocking(fd);

        auto conn = std::make_shared<TcpConnection>(fd, loop_, engine_);
        conn->establish();

        conns_[fd] = conn;
    }
}

void TcpServer::start() {
    loop_->loop();
}
