cmake_minimum_required(VERSION 3.20)
project(kvledit)

set(CMAKE_CXX_STANDARD 20)

add_executable(kvledit kvledit.cpp $<$<BOOL:${WIN32}>:kvledit.rc>)

set_target_properties(kvledit PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-Wl,-rpath='$ORIGIN'" PREFIX "")

target_link_libraries(kvledit PRIVATE game_sdk_shared)

add_dependencies(kvledit assets)

if (WIN32)
    add_dependencies(kvledit copydlls)
    target_sources(kvledit PRIVATE ../app.manifest)
endif()
