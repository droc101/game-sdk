cmake_minimum_required(VERSION 3.20)
project(assets)

set(CMAKE_CXX_STANDARD 20)

set(BUILD_SHARED_LIBS OFF)

include(${CMAKE_SOURCE_DIR}/cmake/FindOrFetchPackage.cmake)

fetchPackage(https://github.com/nlohmann/json.git v3.*.* JSON)

if (WIN32)
    find_package(ZLIB REQUIRED)
    find_package(assimp REQUIRED)
else()
    set(ASSIMP_BUILD_TESTS OFF)
    set(ASSIMP_WARNINGS_AS_ERRORS OFF)
    set(ASSIMP_BUILD_ZLIB ON)
    set(ASSIMP_NO_EXPORT ON)
    set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
    set(ASSIMP_BUILD_OBJ_IMPORTER ON)
    set(ASSIMP_BUILD_FBX_IMPORTER ON)
    set(ASSIMP_BUILD_GLTF_IMPORTER ON)
    set(ASSIMP_BUILD_COLLADA_IMPORTER ON)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
    fetchPackage(https://github.com/assimp/assimp.git v6.*.* assimp)
endif()

set(ENABLE_GLSLANG_BINARIES OFF)
set(ENABLE_HLSL OFF)
set(ENABLE_SPIRV ON)
set(ENABLE_OPT OFF)
fetchPackage(https://github.com/KhronosGroup/glslang.git 16.*.* glslang)

find_package(Vulkan QUIET)
if (Vulkan_INCLUDE_DIRS STREQUAL "Vulkan_INCLUDE_DIR-NOTFOUND")
    fetchPackage(https://github.com/KhronosGroup/Vulkan-Headers.git vulkan-sdk-1.4.*.* VulkanHeaders)
    set(Vulkan_INCLUDE_DIRS ${VULKAN_HEADERS_SOURCE_DIR}/include)
endif ()

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
        src/asset/MapAsset.cpp
        include/libassets/asset/MapAsset.h
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
        src/type/OptionDefinition.cpp
        include/libassets/type/OptionDefinition.h
        src/type/ActorDefinition.cpp
        include/libassets/type/ActorDefinition.h
        src/type/SignalDefinition.cpp
        include/libassets/type/SignalDefinition.h
        include/libassets/type/paramDefs/ParamDefinition.h
        include/libassets/type/paramDefs/ByteParamDefinition.h
        include/libassets/type/paramDefs/IntParamDefinition.h
        include/libassets/type/paramDefs/FloatParamDefinition.h
        include/libassets/type/paramDefs/BoolParamDefinition.h
        include/libassets/type/paramDefs/StringParamDefinition.h
        include/libassets/type/paramDefs/ColorParamDefinition.h
        include/libassets/type/paramDefs/OptionParamDefinition.h
        src/type/paramDefs/ParamDefinition.cpp
        src/asset/LevelMaterialAsset.cpp
        include/libassets/asset/LevelMaterialAsset.h
        include/libassets/type/renderDefs/RenderDefinition.h
        src/type/renderDefs/RenderDefinition.cpp
)

target_link_libraries(assets PUBLIC assimp::assimp glslang::glslang nlohmann_json::nlohmann_json)

target_include_directories(assets PUBLIC ${Vulkan_INCLUDE_DIRS} "include")
target_include_directories(assets PRIVATE ${ASSIMP_INCLUDE_DIRS} "${CMAKE_SOURCE_DIR}/lib/stb")
