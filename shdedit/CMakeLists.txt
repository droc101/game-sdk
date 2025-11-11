cmake_minimum_required(VERSION 3.20)
project(shdedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(shdedit shdedit.cpp
        BatchCompileWindow.cpp
        BatchCompileWindow.h
        BatchDecompileWindow.cpp
        BatchDecompileWindow.h
        $<$<BOOL:${WIN32}>:shdedit.rc>)

set_target_properties(shdedit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(shdedit PRIVATE shared imgui_sdl3_renderer)

add_dependencies(shdedit assets)

if (WIN32)
    add_dependencies(shdedit copydlls)
endif()
