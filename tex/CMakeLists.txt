cmake_minimum_required(VERSION 3.20)
project(tex)

set(CMAKE_CXX_STANDARD 20)

add_executable(tex main.cpp)

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

set_target_properties(tex PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(tex PRIVATE "../libassets/include")

target_link_libraries(tex PRIVATE z ${CMAKE_BINARY_DIR}/libassets/libassets.a)

add_dependencies(tex assets)
