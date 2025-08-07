cmake_minimum_required(VERSION 3.20)
project(fonedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(fonedit fonedit.cpp)

set_target_properties(fonedit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(fonedit PRIVATE shared imgui_sdl3_renderer)

add_dependencies(fonedit assets)

if (WIN32)
    add_dependencies(fonedit copydlls)
endif()
