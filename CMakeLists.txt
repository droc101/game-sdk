cmake_minimum_required(VERSION 3.20)
project(game-sdk)

option(GAME_SDK_BUILD_PLUGINS "Builds GAME SDK plugins" OFF)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Wpedantic -Wall -Wextra -Wno-missing-field-initializers -march=x86-64-v3")

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DBUILDSTYLE_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-g0 -O3 -s -DNDEBUG -DBUILDSTYLE_RELEASE")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O3 -DBUILDSTYLE_DEBUG")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugin) # This puts the libraries built on GitHub actions (such as SDL3) in the bin directory. We need to figure out how to not cause that side effect.

if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT result)
    if (result)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "IPO/LTO is supported")
    else ()
        message(WARNING "IPO/LTO is not supported")
    endif ()
endif ()

if (WIN32)
    message(STATUS "Win32 detected")
    set(CMAKE_AUTO_RCC ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
    execute_process(
            COMMAND cygpath -w /ucrt64
            OUTPUT_VARIABLE MINGW_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(DLLS
            libstdc++-6.dll
            libgcc_s_seh-1.dll
            libwinpthread-1.dll
            SDL3.dll
            libassimp-6.dll
            zlib1.dll
            libminizip-1.dll
            libbz2-1.dll
    )

    add_custom_target(copydlls ALL
            COMMENT "Copying DLLs to build output directory"
    )

    foreach (dll IN LISTS DLLS)
        add_custom_command(TARGET copydlls POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_PATH}\\bin\\${dll}"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\\${dll}"
        )
    endforeach ()
endif ()

include(cmake/FindOrFetchPackage.cmake)

set(SDL_AUDIO_DEFAULT OFF)
set(SDL_GPU_DEFAULT OFF)
set(SDL_CAMERA_DEFAULT OFF)
set(SDL_JOYSTICK_DEFAULT OFF)
set(SDL_HAPTIC_DEFAULT OFF)
set(SDL_POWER_DEFAULT OFF)
set(SDL_SENSOR_DEFAULT OFF)
set(SDL_PIPEWIRE OFF)
set(SDL_OFFSCREEN OFF)
set(SDL_LIBUDEV OFF)
set(SDL_TEST_LIBRARY OFF)
set(SDL_EXAMPLES OFF)
find_or_fetch_package(https://github.com/libsdl-org/SDL.git release-3.*.* SDL3 CONFIG)

fetch_package(https://github.com/mackron/miniaudio.git 0.*.* miniaudio)

fetch_glew()

find_or_fetch_package(https://github.com/g-truc/glm.git 1.*.* glm)

find_package(OpenGL REQUIRED)

add_library(imgui INTERFACE
        mtledit/mtledit.cpp)
target_sources(imgui INTERFACE
        lib/imgui/imconfig.h
        lib/imgui/imgui.cpp
        lib/imgui/imgui.h
        lib/imgui/imgui_demo.cpp
        lib/imgui/imgui_draw.cpp
        lib/imgui/imgui_internal.h
        lib/imgui/imgui_tables.cpp
        lib/imgui/imgui_widgets.cpp
        lib/imgui/imstb_rectpack.h
        lib/imgui/imstb_textedit.h
        lib/imgui/imstb_truetype.h
        lib/imgui/misc/cpp/imgui_stdlib.cpp
        lib/imgui/misc/cpp/imgui_stdlib.h
        lib/imgui/backends/imgui_impl_sdl3.cpp
        lib/imgui/backends/imgui_impl_sdl3.h
)
target_include_directories(imgui INTERFACE
        lib/imgui
        lib/imgui/backends
)

add_library(imgui_sdl3_renderer INTERFACE)
target_link_libraries(imgui_sdl3_renderer INTERFACE imgui)
target_sources(imgui_sdl3_renderer INTERFACE
        lib/imgui/backends/imgui_impl_sdlrenderer3.h
        lib/imgui/backends/imgui_impl_sdlrenderer3.cpp
)

add_library(imgui_opengl3 INTERFACE)
target_link_libraries(imgui_opengl3 INTERFACE imgui)
target_sources(imgui_opengl3 INTERFACE
        lib/imgui/backends/imgui_impl_opengl3.h
        lib/imgui/backends/imgui_impl_opengl3.cpp
        lib/imgui/backends/imgui_impl_opengl3_loader.h
)

add_library(shared INTERFACE)
target_sources(shared INTERFACE
        shared/Options.cpp
        shared/Options.h
        shared/OptionsWindow.cpp
        shared/OptionsWindow.h
        shared/AboutWindow.cpp
        shared/AboutWindow.h
        shared/SharedMgr.cpp
        shared/SharedMgr.h
        shared/TextureBrowserWindow.cpp
        shared/TextureBrowserWindow.h
        shared/ImGuiTextureAssetCache.h
        shared/SDLRendererImGuiTextureAssetCache.cpp
        shared/SDLRendererImGuiTextureAssetCache.h
        shared/OpenGLImGuiTextureAssetCache.cpp
        shared/OpenGLImGuiTextureAssetCache.h
        shared/GLHelper.cpp
        shared/GLHelper.h
        shared/GLDebug.cpp
        shared/GLDebug.h
        shared/DialogFilters.h
        shared/SetupWindow.cpp
        shared/SetupWindow.h
        shared/MaterialBrowserWindow.h
        shared/MaterialBrowserWindow.cpp
        shared/DesktopInterface.h
        shared/DesktopInterface.cpp
)
if (WIN32)
    target_sources(shared INTERFACE app.manifest)
endif ()
target_link_libraries(shared INTERFACE
        SDL3::SDL3
        miniaudio
        assets
        glm::glm
        OpenGL::GL
        glew_s
)
target_include_directories(shared INTERFACE shared)

add_custom_target(
        copy_assets
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin/assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/bin/assets
)

add_subdirectory(libassets)
add_subdirectory(texedit)
add_subdirectory(mdledit)
add_subdirectory(sndedit)
add_subdirectory(mapedit)
add_subdirectory(shdedit)
add_subdirectory(fonedit)
add_subdirectory(cfgedit)
add_subdirectory(mtledit)
add_subdirectory(mapcomp)
add_subdirectory(launcher)
add_dependencies(launcher texedit mdledit sndedit mapedit shdedit fonedit cfgedit mtledit)

if (GAME_SDK_BUILD_PLUGINS)
    if (UNIX)
        add_subdirectory(plugins/gtex_thumbs)
    endif ()
endif ()
