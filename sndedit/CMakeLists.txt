cmake_minimum_required(VERSION 3.20)
project(sndedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(sndedit sndedit.cpp
        SndeditWindow.cpp
        SndeditWindow.h)

configure_sdk_program(sndedit)
