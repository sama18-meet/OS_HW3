cmake_minimum_required(VERSION 3.24)
project(OS_HW3_main C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(OS_HW3_main
        client.c
        output.c
        request.c
        request.h
        segel.c
        segel.h
        server.c)
