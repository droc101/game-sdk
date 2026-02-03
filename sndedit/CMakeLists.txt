cmake_minimum_required(VERSION 3.20)
project(sndedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(sndedit sndedit.cpp $<$<BOOL:${WIN32}>:sndedit.rc>)

set_target_properties(sndedit PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-Wl,-rpath='$ORIGIN'" PREFIX "")

target_link_libraries(sndedit PRIVATE shared)

add_dependencies(sndedit assets)

if (WIN32)
    add_dependencies(sndedit copydlls)
    target_sources(sndedit PRIVATE ../app.manifest)
endif()
