cmake_minimum_required(VERSION 3.20)
project(mapdump)

set(CMAKE_CXX_STANDARD 20)

add_executable(mapdump mapdump.cpp)

configure_sdk_program(mapdump)

target_link_libraries(mapdump PRIVATE glm::glm)
