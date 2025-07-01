cmake_minimum_required(VERSION 3.20)
project(assets)

set(CMAKE_CXX_STANDARD 20)

add_library(assets STATIC include/libassets/libassets.h
        AssetReader.cpp
        include/libassets/AssetReader.h
        include/libassets/TextureAsset.h
        TextureAsset.cpp
        ModelAsset.cpp
        include/libassets/ModelAsset.h
        DataReader.cpp
        include/libassets/DataReader.h
        DataWriter.cpp
        include/libassets/DataWriter.h)

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

find_package(assimp REQUIRED)
include_directories(${ASSIMP_INCLUDE_DIRS})

include_directories("../lib/stb")

target_link_libraries(assets z assimp)
