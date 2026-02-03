cmake_minimum_required(VERSION 3.20)
project(shdedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(shdedit shdedit.cpp
        BatchCompileWindow.cpp
        BatchCompileWindow.h
        BatchDecompileWindow.cpp
        BatchDecompileWindow.h
        $<$<BOOL:${WIN32}>:shdedit.rc>)

set_target_properties(shdedit PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-Wl,-rpath='$ORIGIN'" PREFIX "")

target_link_libraries(shdedit PRIVATE shared)

add_dependencies(shdedit assets)

if (WIN32)
    add_dependencies(shdedit copydlls)
    target_sources(shdedit PRIVATE ../app.manifest)
endif()
