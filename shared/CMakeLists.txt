cmake_minimum_required(VERSION 3.20)
project(shared)

set(CMAKE_CXX_STANDARD 20)

add_library(shared SHARED)
target_sources(shared PRIVATE
        Options.cpp
        Options.h
        OptionsWindow.cpp
        OptionsWindow.h
        AboutWindow.cpp
        AboutWindow.h
        SharedMgr.cpp
        SharedMgr.h
        TextureBrowserWindow.cpp
        TextureBrowserWindow.h
        ImGuiTextureAssetCache.h
        ImGuiTextureAssetCache.cpp
        GLHelper.cpp
        GLHelper.h
        GLDebug.cpp
        GLDebug.h
        DialogFilters.h
        SetupWindow.cpp
        SetupWindow.h
        MaterialBrowserWindow.h
        MaterialBrowserWindow.cpp
        DesktopInterface.h
        DesktopInterface.cpp
        SDKWindow.cpp
        SDKWindow.h
)
if (WIN32)
    target_sources(shared PRIVATE app.manifest)
endif ()
target_link_libraries(shared PUBLIC
        SDL3::SDL3
        miniaudio
        assets
        glm::glm
        OpenGL::GL
        glew_s
        imgui
)
set_target_properties(shared PROPERTIES OUTPUT_NAME "game_sdk" PREFIX "")
target_include_directories(shared PUBLIC ${CMAKE_SOURCE_DIR}/shared)
