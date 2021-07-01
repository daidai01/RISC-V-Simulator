cmake_minimum_required(VERSION 3.17)
project(riscv)

set(CMAKE_CXX_STANDARD 14)

add_executable(riscv main.cpp)

add_executable(test test.cpp)
add_executable(try try.cpp)