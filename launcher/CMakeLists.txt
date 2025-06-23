cmake_minimum_required(VERSION 3.20)
project(sdk-launcher)

set(CMAKE_CXX_STANDARD 20)

add_executable(sdk-launcher main.cpp
        ../lib/imgui/imconfig.h
        ../lib/imgui/imgui.cpp
        ../lib/imgui/imgui.h
        ../lib/imgui/imgui_demo.cpp
        ../lib/imgui/imgui_draw.cpp
        ../lib/imgui/imgui_internal.h
        ../lib/imgui/imgui_tables.cpp
        ../lib/imgui/imgui_widgets.cpp
        ../lib/imgui/imstb_rectpack.h
        ../lib/imgui/imstb_textedit.h
        ../lib/imgui/imstb_truetype.h
        ../lib/imgui/backends/imgui_impl_sdlrenderer3.h
        ../lib/imgui/backends/imgui_impl_sdlrenderer3.cpp
        ../lib/imgui/backends/imgui_impl_sdl3.cpp
        ../lib/imgui/backends/imgui_impl_sdl3.h)

find_package(SDL3 REQUIRED)
include_directories(${SDL3_INCLUDE_DIR})
include_directories("../lib/imgui")
include_directories("../lib/imgui/backends")

target_link_libraries(sdk-launcher ${SDL3_LIBRARY} SDL3)
