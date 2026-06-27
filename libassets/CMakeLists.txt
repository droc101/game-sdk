cmake_minimum_required(VERSION 3.20)
project(assets)

set(CMAKE_CXX_STANDARD 20)

set(BUILD_SHARED_LIBS OFF)

include(${CMAKE_SOURCE_DIR}/cmake/FindOrFetchPackage.cmake)

fetch_package(https://github.com/nlohmann/json.git v3.*.* JSON)

if (WIN32)
    find_package(ZLIB REQUIRED)
    find_package(assimp REQUIRED)
else ()
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
    fetch_package(https://github.com/assimp/assimp.git v6.*.* assimp)
endif ()

if (WIN32)
    find_package(Vulkan QUIET)
else ()
    disable_options(BUILD_WERROR ENABLE_GLSLANG_BINARIES)
    enable_options(ENABLE_SPIRV)
    find_or_fetch_package(https://github.com/KhronosGroup/glslang.git vulkan-sdk-1.4.*.* glslang)

    find_package(Vulkan QUIET COMPONENTS shaderc_combined)

    if (NOT ${Vulkan_shaderc_combined_FOUND})
        find_package(SPIRV-Tools)

        if (NOT ${SpirvTools_FOUND})
            find_or_fetch_package(https://github.com/KhronosGroup/SPIRV-Headers.git vulkan-sdk-1.4.*.* SPIRV-Headers)
            disable_options(SPIRV_WERROR)
            enable_options(SPIRV_SKIP_EXECUTABLES SPIRV_SKIP_TESTS)
            fetch_package(https://github.com/KhronosGroup/SPIRV-Tools.git vulkan-sdk-1.4.*.* SPIRV-Tools)
        endif ()

        disable_options(SHADERC_ENABLE_HLSL SHADERC_ENABLE_WERROR_COMPILE)
        enable_options(SHADERC_SKIP_INSTALL SHADERC_SKIP_TESTS SHADERC_SKIP_EXAMPLES SHADERC_SKIP_EXECUTABLES)
        find_or_fetch_package(https://github.com/google/shaderc.git v2026.2 shaderc)

        add_library(Vulkan::shaderc_combined ALIAS shaderc)
    endif ()
endif ()

if (Vulkan_INCLUDE_DIRS STREQUAL "Vulkan_INCLUDE_DIR-NOTFOUND")
    fetch_package(https://github.com/KhronosGroup/Vulkan-Headers.git vulkan-sdk-1.4.*.* VulkanHeaders)
    set(Vulkan_INCLUDE_DIRS ${VULKAN_HEADERS_SOURCE_DIR}/include)
endif ()

find_or_fetch_package(https://github.com/g-truc/glm.git 1.*.* glm)

set(OPENEXR_INSTALL OFF)
set(OPENEXR_BUILD_TOOLS OFF)
set(OPENEXR_BUILD_EXAMPLES OFF)
set(OPENEXR_TEST_LIBRARIES OFF)
set(OPENEXR_FORCE_INTERNAL_IMATH ON)
set(OPENEXR_FORCE_INTERNAL_OPENJPH ON)
fetch_package(https://github.com/AcademySoftwareFoundation/openexr.git v3.*.* openexr)

fetch_tinyexpr()

add_library(assets SHARED
        include/libassets/libassets.h
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
        src/asset/DataAsset.cpp
        include/libassets/asset/DataAsset.h
        include/libassets/type/paramDefs/Uint64ParamDefinition.h
        include/libassets/type/paramDefs/Vec2ParamDefinition.h
        include/libassets/type/paramDefs/Vec3ParamDefinition.h
        src/util/SearchPathManager.cpp
        include/libassets/util/SearchPathManager.h
        src/util/ActorDefinitionManager.cpp
        include/libassets/util/ActorDefinitionManager.h
        include/libassets/type/MapVertex.h
        src/util/ArgumentParser.cpp
        include/libassets/util/ArgumentParser.h
        include/libassets/util/Logger.h
        include/libassets/type/renderDefs/values/RenderDefinitionValue.h
        src/type/renderDefs/PointRenderDefinition.cpp
        include/libassets/type/renderDefs/PointRenderDefinition.h
        src/type/renderDefs/SpriteRenderDefinition.cpp
        include/libassets/type/renderDefs/SpriteRenderDefinition.h
        src/type/renderDefs/ModelRenderDefinition.cpp
        include/libassets/type/renderDefs/ModelRenderDefinition.h
        src/type/renderDefs/OrientationRenderDefinition.cpp
        include/libassets/type/renderDefs/OrientationRenderDefinition.h
        src/type/renderDefs/BoxRenderDefinition.cpp
        include/libassets/type/renderDefs/BoxRenderDefinition.h
        src/type/renderDefs/WallRenderDefinition.cpp
        include/libassets/type/renderDefs/WallRenderDefinition.h
        src/util/Checksum.cpp
        include/libassets/util/Checksum.h
        src/type/renderDefs/CircleRenderDefinition.cpp
        include/libassets/type/renderDefs/CircleRenderDefinition.h
        src/type/ExpressionParser.cpp
        include/libassets/type/ExpressionParser.h
        include/libassets/type/renderDefs/values/NumericDefinitionValue.h
        src/type/renderDefs/values/StringDefinitionValue.cpp
        include/libassets/type/renderDefs/values/StringDefinitionValue.h
        include/libassets/type/renderDefs/values/BasicDefinitionValue.h
        src/type/renderDefs/values/BoolDefinitionValue.cpp
        include/libassets/type/renderDefs/values/BoolDefinitionValue.h
        src/type/renderDefs/values/ColorDefinitionValue.cpp
        include/libassets/type/renderDefs/values/ColorDefinitionValue.h
        src/type/renderDefs/ConeRenderDefinition.cpp
        include/libassets/type/renderDefs/ConeRenderDefinition.h
)

set_target_properties(assets PROPERTIES
        POSITION_INDEPENDENT_CODE TRUE
        LINK_FLAGS "-Wl,-rpath='$ORIGIN'"
)

target_link_libraries(assets PUBLIC assimp::assimp nlohmann_json::nlohmann_json glm::glm OpenEXR::OpenEXR tinyexpr)
if (WIN32)
    target_link_libraries(assets PUBLIC shaderc_shared)
else ()
    target_link_libraries(assets PUBLIC glslang::glslang Vulkan::shaderc_combined)
endif ()

target_include_directories(assets PUBLIC ${Vulkan_INCLUDE_DIRS} "include" "${CMAKE_SOURCE_DIR}/lib/stb")
