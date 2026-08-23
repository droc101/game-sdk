cmake_minimum_required(VERSION 3.20)
project(kvledit)

set(CMAKE_CXX_STANDARD 20)

add_executable(kvledit kvledit.cpp
        KvleditWindow.cpp
        KvleditWindow.h)

configure_sdk_program(kvledit)
