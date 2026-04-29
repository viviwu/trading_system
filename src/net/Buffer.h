/**
  ******************************************************************************
  * @file           : Buffer.h
  * @author         : vivi wu 
  * @brief          : None
  * @version        : 0.1.0
  * @date           : 30/04/26
  ******************************************************************************
  */


#ifndef TRADING_SYSTEM_BUFFER_H
#define TRADING_SYSTEM_BUFFER_H



#pragma once
#include <string>

class Buffer {
    std::string buf_;

public:
    void append(const char* data, size_t len);
    const char* data() const;
    size_t size() const;
    void retrieve(size_t len);
    bool empty() const;
};


#endif //TRADING_SYSTEM_BUFFER_H
