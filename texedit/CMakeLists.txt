cmake_minimum_required(VERSION 3.20)
project(texedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(texedit texedit.cpp
        TexeditWindow.cpp
        TexeditWindow.h)

configure_sdk_gui_program(texedit)
