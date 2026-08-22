cmake_minimum_required(VERSION 3.20)
project(shdedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(shdedit shdedit.cpp $<$<BOOL:${WIN32}>:shdedit.rc>
        ShdeditWindow.cpp
        ShdeditWindow.h)

set_target_properties(shdedit PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-Wl,-rpath='$ORIGIN'" PREFIX "")

target_link_libraries(shdedit PRIVATE game_sdk_shared)

add_dependencies(shdedit assets)

if (WIN32)
    add_dependencies(shdedit copydlls)
    target_sources(shdedit PRIVATE ../app.manifest)
endif()
