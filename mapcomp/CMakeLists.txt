cmake_minimum_required(VERSION 3.20)
project(mapcomp)

set(CMAKE_CXX_STANDARD 20)

fetch_earcut_hpp()

add_executable(mapcomp mapcomp.cpp
        $<$<BOOL:${WIN32}>:mapcomp.rc>
        ArgumentParser.cpp
        ArgumentParser.h
        LevelMeshBuilder.cpp
        LevelMeshBuilder.h
        MapCompiler.cpp
        MapCompiler.h
        SectorCollisionBuilder.cpp
        SectorCollisionBuilder.h
)

set_target_properties(mapcomp PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-Wl,-rpath='$ORIGIN'" PREFIX "")

target_link_libraries(mapcomp PRIVATE
        earcut
        assets
        glm::glm
)

add_dependencies(mapcomp assets)

if (WIN32)
    add_dependencies(mapcomp copydlls)
endif ()
