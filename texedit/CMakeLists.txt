cmake_minimum_required(VERSION 3.20)
project(texedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(texedit main.cpp
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
        ../lib/imgui/misc/cpp/imgui_stdlib.cpp
        ../lib/imgui/misc/cpp/imgui_stdlib.h
        ../lib/imgui/backends/imgui_impl_sdlrenderer3.h
        ../lib/imgui/backends/imgui_impl_sdlrenderer3.cpp
        ../lib/imgui/backends/imgui_impl_sdl3.cpp
        ../lib/imgui/backends/imgui_impl_sdl3.h
        ../shared/Options.cpp
        ../shared/Options.h
        ../shared/OptionsWindow.cpp
        ../shared/OptionsWindow.h
        ../shared/AboutWindow.cpp
        ../shared/AboutWindow.h
        ../shared/SharedMgr.cpp
        ../shared/SharedMgr.h)

find_package(SDL3 REQUIRED)
include_directories(${SDL3_INCLUDE_DIR})

include_directories("../lib/imgui")
include_directories("../lib/imgui/backends")

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

include_directories("../lib/json/include")
include_directories("../libassets/include")
include_directories("../shared")

set_target_properties(texedit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(texedit PRIVATE ${SDL3_LIBRARY} SDL3 z nlohmann_json::nlohmann_json  ${CMAKE_BINARY_DIR}/libassets/libassets.a)

add_dependencies(texedit assets)
