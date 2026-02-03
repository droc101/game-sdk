cmake_minimum_required(VERSION 3.20)
project(fonedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(fonedit fonedit.cpp $<$<BOOL:${WIN32}>:fonedit.rc>)

set_target_properties(fonedit PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-Wl,-rpath='$ORIGIN'" PREFIX "")

target_link_libraries(fonedit PRIVATE game_sdk_shared)

add_dependencies(fonedit assets)

if (WIN32)
    add_dependencies(fonedit copydlls)
    target_sources(fonedit PRIVATE ../app.manifest)
endif()
