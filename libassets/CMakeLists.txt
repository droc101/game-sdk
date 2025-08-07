cmake_minimum_required(VERSION 3.20)
project(assets)

set(CMAKE_CXX_STANDARD 20)

find_package(ZLIB REQUIRED)
find_package(assimp REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

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
        include/libassets/util/Primitive.h
        include/libassets/util/Asset.h
        src/util/ModelVertex.cpp
        include/libassets/util/ModelVertex.h
        src/util/Material.cpp
        include/libassets/util/Material.h
        src/util/ModelLod.cpp
        include/libassets/util/ModelLod.h
        include/libassets/util/Error.h
        src/util/ShaderCompiler.cpp
        include/libassets/util/ShaderCompiler.h
        src/asset/ShaderAsset.cpp
        include/libassets/asset/ShaderAsset.h
        src/asset/FontAsset.cpp
        include/libassets/asset/FontAsset.h
        include/libassets/util/VectorMove.h
)

target_link_libraries(assets PRIVATE ZLIB::ZLIB assimp::assimp glslang::glslang Vulkan::Vulkan)

target_include_directories(assets PUBLIC "include")
target_include_directories(assets PRIVATE ${ZLIB_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIRS} "../lib/stb")
