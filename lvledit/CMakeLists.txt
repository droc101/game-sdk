cmake_minimum_required(VERSION 3.20)
project(lvledit)

set(CMAKE_CXX_STANDARD 20)

add_executable(lvledit lvledit.cpp
        Viewport.cpp
        Viewport.h
        LevelRenderer.cpp
        LevelRenderer.h
        LevelEditor.cpp
        LevelEditor.h
        tools/EditorTool.h
        tools/VertexTool.cpp
        tools/VertexTool.h)

set_target_properties(lvledit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(lvledit PRIVATE shared imgui_opengl3)

add_dependencies(lvledit assets)

add_dependencies(lvledit copy_assets)

if (WIN32)
    add_dependencies(lvledit copydlls)
endif()
