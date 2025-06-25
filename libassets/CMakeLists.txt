cmake_minimum_required(VERSION 3.20)
project(assets)

add_library(assets STATIC include/libassets/libassets.h
        AssetReader.cpp
        include/libassets/AssetReader.h
        include/libassets/TextureAsset.h
        TextureAsset.cpp)

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories("../lib/stb")
target_link_libraries(assets z)
