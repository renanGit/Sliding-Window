cmake_minimum_required(VERSION 3.25)

project(server LANGUAGES CXX)

add_library(send_recv STATIC send_recv.cpp)
add_executable(server server.cpp)

target_link_libraries(server PRIVATE send_recv)