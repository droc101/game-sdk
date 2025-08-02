cmake_minimum_required(VERSION 3.20)
project(shdedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(shdedit shdedit.cpp
        ${IMGUI_SRC}
        ${IMGUI_SRC_SDL3_RENDERER}
        ${SHARED_SRC}
)

include_directories(${SHARED_INCLUDES})

set_target_properties(shdedit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(shdedit PRIVATE ${SHARED_LIBRARIES})

add_dependencies(shdedit assets)

if (WIN32)
    add_dependencies(shdedit copydlls)
endif()
