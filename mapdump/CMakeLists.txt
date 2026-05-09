cmake_minimum_required(VERSION 3.20)
project(mapdump)

set(CMAKE_CXX_STANDARD 20)

add_executable(mapdump mapdump.cpp
        $<$<BOOL:${WIN32}>:mapdump.rc>
)

set_target_properties(mapdump PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-Wl,-rpath='$ORIGIN'" PREFIX "")

target_link_libraries(mapdump PRIVATE
        assets
        glm::glm
)

if (WIN32)
    add_dependencies(mapdump copydlls)
endif ()
