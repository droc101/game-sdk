cmake_minimum_required(VERSION 3.20)
project(mtledit)

set(CMAKE_CXX_STANDARD 20)

add_executable(mtledit mtledit.cpp
        MtleditWindow.cpp
        MtleditWindow.h)

configure_sdk_program(mtledit)