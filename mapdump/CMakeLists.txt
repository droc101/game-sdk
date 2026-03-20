cmake_minimum_required(VERSION 3.20)
project(mapdump)

set(CMAKE_CXX_STANDARD 20)

set(OPENEXR_INSTALL OFF)
set(OPENEXR_BUILD_TOOLS OFF)
set(OPENEXR_BUILD_EXAMPLES OFF)
set(OPENEXR_TEST_LIBRARIES OFF)
fetch_package(https://github.com/AcademySoftwareFoundation/openexr.git v3.*.* openexr)

add_executable(mapdump mapdump.cpp
        $<$<BOOL:${WIN32}>:mapdump.rc>
)

set_target_properties(mapdump PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-Wl,-rpath='$ORIGIN'" PREFIX "")

target_link_libraries(mapdump PRIVATE
        assets
        glm::glm
        OpenEXR::OpenEXR
)

if (WIN32)
    add_dependencies(mapdump copydlls)
endif ()
