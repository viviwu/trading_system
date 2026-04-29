/**
  ******************************************************************************
  * @file           : Buffer.cpp
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#include "Buffer.h"

void Buffer::append(const char* data, size_t len) {
    buf_.append(data, len);
}

const char* Buffer::data() const {
    return buf_.data();
}

size_t Buffer::size() const {
    return buf_.size();
}

void Buffer::retrieve(size_t len) {
    buf_.erase(0, len);
}

bool Buffer::empty() const {
    return buf_.empty();
}
