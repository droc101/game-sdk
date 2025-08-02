cmake_minimum_required(VERSION 3.20)
project(texedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(texedit texedit.cpp
        ${IMGUI_SRC}
        ${IMGUI_SRC_SDL3_RENDERER}
        ${SHARED_SRC}
)

include_directories(${SHARED_INCLUDES})

set_target_properties(texedit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(texedit PRIVATE ${SHARED_LIBRARIES})

add_dependencies(texedit assets)

if (WIN32)
    add_dependencies(texedit copydlls)
endif()
