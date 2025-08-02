cmake_minimum_required(VERSION 3.20)
project(lvledit)

set(CMAKE_CXX_STANDARD 20)

add_executable(lvledit lvledit.cpp
        ${IMGUI_SRC}
        ${IMGUI_SRC_SDL3_RENDERER}
        ${SHARED_SRC}
)

include_directories(${SHARED_INCLUDES})

set_target_properties(lvledit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(lvledit PRIVATE ${SHARED_LIBRARIES})

add_dependencies(lvledit assets)

if (WIN32)
    add_dependencies(lvledit copydlls)
endif()
