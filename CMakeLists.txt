cmake_minimum_required(VERSION 3.16)
project(trading_system)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O2 -pthread")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

include_directories(src)

file(GLOB_RECURSE SRC_FILES
  src/net/*.cpp
  src/engine/*.cpp
  src/main_server.cpp
)

add_executable(server ${SRC_FILES})

add_executable(client
  client/main_client.cpp
)