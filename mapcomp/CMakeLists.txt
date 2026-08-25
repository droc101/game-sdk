cmake_minimum_required(VERSION 3.20)
project(mapcomp)

set(CMAKE_CXX_STANDARD 23)

fetch_earcut_hpp()

fetch_clipper2()

FetchContent_Declare(
        Luna
        GIT_REPOSITORY https://github.com/NBT22/Luna
        GIT_TAG 4902420b47ef06d6dc4e67fb3af2c6a989e9d6a1
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(Luna)

add_executable(mapcomp mapcomp.cpp
        LevelMeshBuilder.cpp
        LevelMeshBuilder.h
        MapCompiler.cpp
        MapCompiler.h
        SectorCollisionBuilder.cpp
        SectorCollisionBuilder.h
        SectorClipper.cpp
        SectorClipper.h
        Light.h
        LightBaker.cpp
        LightBaker.hpp
        LightBakerGpu.cpp
        LightBakerGpu.hpp
)

configure_sdk_program(mapcomp)

target_link_libraries(mapcomp PRIVATE
        earcut
        Clipper2
        glm::glm
        Luna
)
