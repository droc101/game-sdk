cmake_minimum_required(VERSION 3.20)
project(mdledit)

set(CMAKE_CXX_STANDARD 20)

add_executable(mdledit main.cpp
        ModelRenderer.cpp
        ModelRenderer.h
        SkinEditWindow.cpp
        SkinEditWindow.h
        LodEditWindow.cpp
        LodEditWindow.h
        ${IMGUI_SRC}
        ${SHARED_SRC}
        ${IMGUI_SRC_OPENGL3}
        GLDebug.cpp
        GLDebug.h
)

include_directories(${SHARED_INCLUDES})

set_target_properties(mdledit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(mdledit PRIVATE ${SHARED_LIBRARIES})

add_dependencies(mdledit assets)

add_dependencies(mdledit copy_assets)

if (WIN32)
    add_dependencies(mdledit copydlls)
endif()
