cmake_minimum_required(VERSION 3.20)
project(launcher)

set(CMAKE_CXX_STANDARD 20)

add_executable(launcher launcher.cpp $<$<BOOL:${WIN32}>:launcher.rc>)

set_target_properties(launcher PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(launcher PRIVATE shared imgui_sdl3_renderer)

add_dependencies(launcher assets copy_assets)

if (WIN32)
    add_dependencies(launcher copydlls)
endif()
