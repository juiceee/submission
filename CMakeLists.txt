cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -fpermissive")

project(socket)
file(GLOB_RECURSE server server.cpp server.h)
file(GLOB_RECURSE client client.cpp server.h)

add_executable(client_exe    ${client})
add_executable(server_exe  ${server})
