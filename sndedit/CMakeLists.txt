cmake_minimum_required(VERSION 3.20)
project(sndedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(sndedit sndedit.cpp)

set_target_properties(sndedit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(sndedit PRIVATE shared imgui_sdl3_renderer)

add_dependencies(sndedit assets)

if (WIN32)
    add_dependencies(sndedit copydlls)
endif()
