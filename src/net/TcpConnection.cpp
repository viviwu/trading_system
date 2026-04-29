/**
  ******************************************************************************
  * @file           : TcpConnection.cpp
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "../protocol/Protocol.h"
#include "../engine/MatchingEngine.h"

#include <unistd.h>
#include <errno.h>
#include <cstring>

TcpConnection::TcpConnection(int fd, EventLoop* loop, MatchingEngine* engine)
    : fd_(fd), loop_(loop), engine_(engine) {

    channel_ = std::make_unique<Channel>(loop_, fd_);

    channel_->setReadCallback([this]() { handleRead(); });
    channel_->setWriteCallback([this]() { handleWrite(); });
}

void TcpConnection::establish() {
    channel_->enableRead();
}

void TcpConnection::handleRead() {
    char buf[4096];

    while (true) {
        int n = read(fd_, buf, sizeof(buf));

        if (n > 0) {
            input_.append(buf, n);
        } else if (n == 0) {
            handleClose();
            return;
        } else {
            if (errno == EAGAIN) break;
            handleClose();
            return;
        }
    }

    while (input_.size() >= sizeof(OrderReq)) {
        OrderReq req;
        memcpy(&req, input_.data(), sizeof(req));
        input_.retrieve(sizeof(req));

        engine_->submit(req, [self = shared_from_this()](OrderResp resp) {
            self->send((char*)&resp, sizeof(resp));
        });
    }
}

void TcpConnection::handleWrite() {
    while (!output_.empty()) {
        int n = write(fd_, output_.data(), output_.size());

        if (n > 0) {
            output_.retrieve(n);
        } else {
            if (errno == EAGAIN) break;
            handleClose();
            return;
        }
    }

    if (output_.empty()) {
        channel_->disableWrite();
    }
}

void TcpConnection::handleClose() {
    close(fd_);
}

void TcpConnection::send(const char* data, size_t len) {
    output_.append(data, len);
    channel_->enableWrite();
}