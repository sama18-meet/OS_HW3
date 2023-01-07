cmake_minimum_required(VERSION 3.24)
project(OS_HW3_main C)

set(CMAKE_C_STANDARD 99)

add_executable(OS_HW3_main segel.c segel.h server.c request.c request.h)
