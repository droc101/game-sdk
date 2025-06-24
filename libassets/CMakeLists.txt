cmake_minimum_required(VERSION 3.20)
project(assets)

add_library(assets STATIC include/libassets/libassets.h
        Asset.cpp
        include/libassets/Asset.h
        include/libassets/RawAsset.h
        include/libassets/TextureAsset.h
        RawAsset.cpp
        TextureAsset.cpp)

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories("../lib/stb")
target_link_libraries(assets z)
