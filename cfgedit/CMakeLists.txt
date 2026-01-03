cmake_minimum_required(VERSION 3.20)
project(cfgedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(cfgedit cfgedit.cpp $<$<BOOL:${WIN32}>:cfgedit.rc>)

set_target_properties(cfgedit PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-Wl,-rpath='$ORIGIN'" PREFIX "")

target_link_libraries(cfgedit PRIVATE shared imgui_sdl3_renderer)

add_dependencies(cfgedit assets)

if (WIN32)
    add_dependencies(cfgedit copydlls)
endif()
