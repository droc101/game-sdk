cmake_minimum_required(VERSION 3.20)
project(assets)

set(CMAKE_CXX_STANDARD 20)

add_library(assets STATIC include/libassets/libassets.h
        src/util/AssetReader.cpp
        include/libassets/util/AssetReader.h
        include/libassets/asset/TextureAsset.h
        src/asset/TextureAsset.cpp
        src/asset/ModelAsset.cpp
        include/libassets/asset/ModelAsset.h
        src/util/DataReader.cpp
        include/libassets/util/DataReader.h
        src/util/DataWriter.cpp
        include/libassets/util/DataWriter.h
        src/asset/SoundAsset.cpp
        include/libassets/asset/SoundAsset.h
        src/asset/LevelAsset.cpp
        include/libassets/asset/LevelAsset.h
        src/util/Color.cpp
        include/libassets/util/Color.h
)

find_package(ZLIB REQUIRED)
find_package(assimp REQUIRED)

include_directories("include")
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(${ASSIMP_INCLUDE_DIRS})
include_directories("../lib/stb")

target_link_libraries(assets ZLIB::ZLIB assimp::assimp)
