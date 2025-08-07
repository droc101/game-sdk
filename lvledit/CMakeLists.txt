cmake_minimum_required(VERSION 3.20)
project(lvledit)

set(CMAKE_CXX_STANDARD 20)

add_executable(lvledit lvledit.cpp)

set_target_properties(lvledit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(lvledit PRIVATE shared imgui_sdl3_renderer)

add_dependencies(lvledit assets)

if (WIN32)
    add_dependencies(lvledit copydlls)
endif()
