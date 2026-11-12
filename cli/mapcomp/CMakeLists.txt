cmake_minimum_required(VERSION 3.20)
project(mapcomp)

set(CMAKE_CXX_STANDARD 23)

FetchContent_Declare(
        Luna
        GIT_REPOSITORY https://github.com/NBT22/Luna
        GIT_TAG 93cb73600b0a60aab7ffd65f255112d01315ae9f
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
)

if (ENABLE_MAPCOMP_RENDERDOC)
    target_compile_definitions(mapcomp PRIVATE ENABLE_RENDERDOC)
endif ()

configure_sdk_cli_program(mapcomp)

target_link_libraries(mapcomp PRIVATE
        glm::glm
        Luna
)
