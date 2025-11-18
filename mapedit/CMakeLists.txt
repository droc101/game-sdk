cmake_minimum_required(VERSION 3.20)
project(mapedit)

set(CMAKE_CXX_STANDARD 20)

add_executable(mapedit mapedit.cpp
        Viewport.cpp
        Viewport.h
        MapRenderer.cpp
        MapRenderer.h
        MapEditor.cpp
        MapEditor.h
        tools/EditorTool.h
        tools/AddPolygonTool.cpp
        tools/AddPolygonTool.h
        tools/AddPrimitiveTool.cpp
        tools/AddPrimitiveTool.h
        tools/SelectTool.cpp
        tools/SelectTool.h
        ActorBrowserWindow.cpp
        ActorBrowserWindow.h
        tools/AddActorTool.cpp
        tools/AddActorTool.h
        EditActorWindow.cpp
        EditActorWindow.h
        $<$<BOOL:${WIN32}>:mapedit.rc>
        MapPropertiesWindow.cpp
        MapPropertiesWindow.h
        MapCompileWindow.cpp
        MapCompileWindow.h
)

set_target_properties(mapedit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(mapedit PRIVATE shared imgui_opengl3)

add_dependencies(mapedit assets copy_assets mapcomp)

if (WIN32)
    add_dependencies(mapedit copydlls)
endif()
