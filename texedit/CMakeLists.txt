cmake_minimum_required(VERSION 3.20)
project(texedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(texedit texedit.cpp $<$<BOOL:${WIN32}>:texedit.rc>)

set_target_properties(texedit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(texedit PRIVATE shared imgui_sdl3_renderer)

add_dependencies(texedit assets)

if (WIN32)
    add_dependencies(texedit copydlls)
endif()
