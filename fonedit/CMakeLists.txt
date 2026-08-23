cmake_minimum_required(VERSION 3.20)
project(fonedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(fonedit fonedit.cpp
        FoneditWindow.cpp
        FoneditWindow.h)

configure_sdk_program(fonedit)
