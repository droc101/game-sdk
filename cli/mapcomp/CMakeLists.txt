cmake_minimum_required(VERSION 3.20)
project(mapcomp)

set(CMAKE_CXX_STANDARD 23)

FetchContent_Declare(
        Luna
        GIT_REPOSITORY https://github.com/NBT22/Luna
        GIT_TAG b374cb2f6c479abe02dd4a882da7789528248e44
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(Luna)

add_executable(mapcomp mapcomp.cpp
        BrushMeshBuilder.cpp
        BrushMeshBuilder.h
        MapCompiler.cpp
        MapCompiler.h
        BrushCollisionBuilder.cpp
        BrushCollisionBuilder.h
        Light.h
        LightBaker.cpp
        LightBaker.hpp
        Light.cpp
        LightmapPacker.cpp
        LightmapPacker.h
        RenderdocManager.cpp
        RenderdocManager.h
        NsightManager.cpp
        NsightManager.h
)

if (ENABLE_MAPCOMP_RENDERDOC)
    target_compile_definitions(mapcomp PRIVATE ENABLE_RENDERDOC)
endif ()

if (ENABLE_MAPCOMP_NSIGHT)
    set(NSIGHT_GRAPHICS_SDK_INCLUDE_DIR "${NSIGHT_GRAPHICS_SDK_DIR}/include")
    if (NOT EXISTS "${NSIGHT_GRAPHICS_SDK_INCLUDE_DIR}/NGFX_GraphicsCapture_Vulkan.h")
        message(FATAL_ERROR "NGFX_GraphicsCapture_Vulkan.h not found in ${NSIGHT_GRAPHICS_SDK_INCLUDE_DIR}. Make sure you define NSIGHT_GRAPHICS_SDK_DIR")
    endif ()

    target_compile_definitions(mapcomp PRIVATE ENABLE_NSIGHT)
    target_include_directories(mapcomp SYSTEM PRIVATE "${NSIGHT_GRAPHICS_SDK_INCLUDE_DIR}")
    if (NOT WIN32)
        target_link_libraries(mapcomp PRIVATE ${CMAKE_DL_LIBS})
    endif ()
endif ()

configure_sdk_cli_program(mapcomp)

target_link_libraries(mapcomp PRIVATE
        glm::glm
        Luna
)
