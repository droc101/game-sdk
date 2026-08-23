cmake_minimum_required(VERSION 3.20)
project(shdedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(shdedit shdedit.cpp
        ShdeditWindow.cpp
        ShdeditWindow.h)

configure_sdk_program(shdedit)