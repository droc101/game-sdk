cmake_minimum_required(VERSION 3.20)
project(mapcomp)

set(CMAKE_CXX_STANDARD 23)

fetch_earcut_hpp()

fetch_clipper2()

FetchContent_Declare(
        Luna
        GIT_REPOSITORY https://github.com/NBT22/Luna
        GIT_TAG 0.3.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(Luna)

add_executable(mapcomp mapcomp.cpp
        $<$<BOOL:${WIN32}>:mapcomp.rc>
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
        LightBakerCpu.cpp
        LightBakerCpu.hpp
)

target_compile_options(mapcomp PRIVATE -pthread)

set_target_properties(mapcomp PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-Wl,-rpath='$ORIGIN'" PREFIX "")

target_link_libraries(mapcomp PRIVATE
        earcut
        Clipper2
        assets
        glm::glm
        Luna
)

add_dependencies(mapcomp assets copy_assets)

if (WIN32)
    add_dependencies(mapcomp copydlls)
endif ()
