cmake_minimum_required(VERSION 3.25)

project(client LANGUAGES CXX)

add_library(send_recv STATIC send_recv.cpp)
add_executable(client client.cpp)

target_link_libraries(client PRIVATE send_recv)