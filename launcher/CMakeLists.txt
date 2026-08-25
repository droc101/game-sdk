cmake_minimum_required(VERSION 3.20)
project(launcher)

set(CMAKE_CXX_STANDARD 20)

add_executable(launcher launcher.cpp
        LauncherWindow.cpp
        LauncherWindow.h)

configure_sdk_gui_program(launcher)