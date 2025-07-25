cmake_minimum_required(VERSION 3.20)
project(fonedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(fonedit main.cpp
        ${IMGUI_SRC}
        ${IMGUI_SRC_SDL3_RENDERER}
        ${SHARED_SRC}
)

include_directories(${SHARED_INCLUDES})

set_target_properties(fonedit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(fonedit PRIVATE ${SHARED_LIBRARIES})

add_dependencies(fonedit assets)

if (WIN32)
    add_dependencies(fonedit copydlls)
endif()
