cmake_minimum_required(VERSION 3.20)
project(assets)

set(CMAKE_CXX_STANDARD 20)

find_package(ZLIB REQUIRED)
find_package(assimp REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

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
        src/type/Color.cpp
        include/libassets/type/Color.h
        include/libassets/util/Primitive.h
        include/libassets/type/Asset.h
        src/type/ModelVertex.cpp
        include/libassets/type/ModelVertex.h
        src/type/Material.cpp
        include/libassets/type/Material.h
        src/type/ModelLod.cpp
        include/libassets/type/ModelLod.h
        include/libassets/util/Error.h
        src/util/ShaderCompiler.cpp
        include/libassets/util/ShaderCompiler.h
        src/asset/ShaderAsset.cpp
        include/libassets/asset/ShaderAsset.h
        src/asset/FontAsset.cpp
        include/libassets/asset/FontAsset.h
        include/libassets/util/VectorMove.h
        src/type/BoundingBox.cpp
        include/libassets/type/BoundingBox.h
        src/type/ConvexHull.cpp
        include/libassets/type/ConvexHull.h
        src/type/StaticCollisionMesh.cpp
        include/libassets/type/StaticCollisionMesh.h
        src/type/Sector.cpp
        include/libassets/type/Sector.h
        src/type/WallMaterial.cpp
        include/libassets/type/WallMaterial.h
        src/type/Param.cpp
        include/libassets/type/Param.h
        src/type/Actor.cpp
        include/libassets/type/Actor.h
        src/type/IOConnection.cpp
        include/libassets/type/IOConnection.h
        src/asset/GameConfigAsset.cpp
        include/libassets/asset/GameConfigAsset.h
        src/asset/GameConfigAsset.cpp
        include/libassets/asset/GameConfigAsset.h
)

target_link_libraries(assets PRIVATE ZLIB::ZLIB assimp::assimp glslang::glslang Vulkan::Vulkan)

target_include_directories(assets PUBLIC "include")
target_include_directories(assets PRIVATE ${ZLIB_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIRS} "../lib/stb")
