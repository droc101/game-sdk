cmake_minimum_required(VERSION 3.20)
project(mtledit)

set(CMAKE_CXX_STANDARD 20)

add_executable(mtledit mtledit.cpp $<$<BOOL:${WIN32}>:mtledit.rc>)

set_target_properties(mtledit PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-Wl,-rpath='$ORIGIN'" PREFIX "")

target_link_libraries(mtledit PRIVATE shared imgui_sdl3_renderer)

add_dependencies(mtledit assets)

if (WIN32)
    add_dependencies(mtledit copydlls)
endif()
