cmake_minimum_required(VERSION 3.20)
project(shdcomp)

set(CMAKE_CXX_STANDARD 20)

add_executable(shdcomp shdcomp.cpp)

configure_sdk_cli_program(shdcomp)
