cmake_minimum_required(VERSION 3.20)
project(game_sdk_shared)

set(CMAKE_CXX_STANDARD 20)

add_library(game_sdk_shared SHARED)
target_sources(game_sdk_shared PRIVATE
        src/Options.cpp
        include/game_sdk/Options.h
        src/DesktopInterface.cpp
        include/game_sdk/DesktopInterface.h
        src/SDKWindow.cpp
        include/game_sdk/SDKWindow.h
        src/SharedMgr.cpp
        include/game_sdk/SharedMgr.h
        src/gl/GLDebug.cpp
        include/game_sdk/gl/GLDebug.h
        src/gl/GLHelper.cpp
        include/game_sdk/gl/GLHelper.h
        src/gl/GLTextureCache.cpp
        include/game_sdk/gl/GLTextureCache.h
        src/windows/AboutWindow.cpp
        include/game_sdk/windows/AboutWindow.h
        src/windows/MaterialBrowserWindow.cpp
        include/game_sdk/windows/MaterialBrowserWindow.h
        src/windows/OptionsWindow.cpp
        include/game_sdk/windows/OptionsWindow.h
        src/windows/SetupWindow.cpp
        include/game_sdk/windows/SetupWindow.h
        src/windows/TextureBrowserWindow.cpp
        include/game_sdk/windows/TextureBrowserWindow.h
)
target_link_libraries(game_sdk_shared PRIVATE glew_s imgui)
target_link_libraries(game_sdk_shared PUBLIC
        SDL3::SDL3
        glm::glm
        OpenGL::GL
        assets
        miniaudio
        imgui_headers
)
set_target_properties(game_sdk_shared PROPERTIES
        OUTPUT_NAME "game_sdk"
        PREFIX ""
        POSITION_INDEPENDENT_CODE ON
        LINK_FLAGS "-Wl,-rpath='$ORIGIN'"
)
target_include_directories(game_sdk_shared PUBLIC ${CMAKE_SOURCE_DIR}/game_sdk_shared/include)
