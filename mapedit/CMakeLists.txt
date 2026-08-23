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
        MapPropertiesWindow.cpp
        MapPropertiesWindow.h
        MapCompileWindow.cpp
        MapCompileWindow.h
        ViewportRenderer.cpp
        ViewportRenderer.h
        MapeditWindow.cpp
        MapeditWindow.h
        CompileProgressWindow.cpp
        CompileProgressWindow.h
)

configure_sdk_program(mapedit)
add_dependencies(mapedit mapcomp)
