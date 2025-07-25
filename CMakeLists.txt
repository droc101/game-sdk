cmake_minimum_required(VERSION 3.20)
project(game-sdk)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Wpedantic -Wall -Wextra -Wno-missing-field-initializers")

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DBUILDSTYLE_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-g0 -O3 -s -DNDEBUG -DBUILDSTYLE_RELEASE")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O3 -DBUILDSTYLE_DEBUG")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (WIN32)
    message(STATUS "Win32 detected")
    set(CMAKE_AUTO_RCC ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
    execute_process(
            COMMAND cygpath -w /mingw64
            OUTPUT_VARIABLE MINGW_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(DLLS
            libstdc++-6.dll
            libgcc_s_seh-1.dll
            libwinpthread-1.dll
            SDL3.dll
            glew32.dll
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

set(IMGUI_SRC
        ../lib/imgui/imconfig.h
        ../lib/imgui/imgui.cpp
        ../lib/imgui/imgui.h
        ../lib/imgui/imgui_demo.cpp
        ../lib/imgui/imgui_draw.cpp
        ../lib/imgui/imgui_internal.h
        ../lib/imgui/imgui_tables.cpp
        ../lib/imgui/imgui_widgets.cpp
        ../lib/imgui/imstb_rectpack.h
        ../lib/imgui/imstb_textedit.h
        ../lib/imgui/imstb_truetype.h
        ../lib/imgui/misc/cpp/imgui_stdlib.cpp
        ../lib/imgui/misc/cpp/imgui_stdlib.h
        ../lib/imgui/backends/imgui_impl_sdl3.cpp
        ../lib/imgui/backends/imgui_impl_sdl3.h
)

set(IMGUI_SRC_SDL3_RENDERER
        ../lib/imgui/backends/imgui_impl_sdlrenderer3.h
        ../lib/imgui/backends/imgui_impl_sdlrenderer3.cpp
)

set(IMGUI_SRC_OPENGL3
        ../lib/imgui/backends/imgui_impl_opengl3.h
        ../lib/imgui/backends/imgui_impl_opengl3.cpp
        ../lib/imgui/backends/imgui_impl_opengl3_loader.h
)

set(SHARED_SRC
        ../shared/Options.cpp
        ../shared/Options.h
        ../shared/OptionsWindow.cpp
        ../shared/OptionsWindow.h
        ../shared/AboutWindow.cpp
        ../shared/AboutWindow.h
        ../shared/SharedMgr.cpp
        ../shared/SharedMgr.h
        ../shared/TextureBrowserWindow.cpp
        ../shared/TextureBrowserWindow.h
        ../shared/ImGuiTextureAssetCache.h
        ../shared/SDLRendererImGuiTextureAssetCache.cpp
        ../shared/SDLRendererImGuiTextureAssetCache.h
        ../shared/OpenGLImGuiTextureAssetCache.cpp
        ../shared/OpenGLImGuiTextureAssetCache.h
)

if (WIN32)
    set(SHARED_SRC
            ${SHARED_SRC}
            ../game-sdk.rc
    )
endif ()

include(cmake/FindOrFetchPackage.cmake)

set(SDL_AUDIO_DEFAULT OFF)
set(SDL_CAMERA_DEFAULT OFF)
set(SDL_JOYSTICK_DEFAULT OFF)
set(SDL_HAPTIC_DEFAULT OFF)
set(SDL_POWER_DEFAULT OFF)
set(SDL_SENSOR_DEFAULT OFF)
set(SDL_TEST_LIBRARY OFF)
set(SDL_PIPEWIRE OFF)
set(SDL_OFFSCREEN OFF)
findOrFetchPackage(https://github.com/libsdl-org/SDL.git release-3.*.* SDL3 CONFIG)
fetchPackage(https://github.com/nlohmann/json.git v3.*.* JSON)
fetchPackage(https://github.com/mackron/miniaudio.git 0.*.* miniaudio)

set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)
set(ASSIMP_BUILD_ZLIB OFF)
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_OBJ_IMPORTER ON)
set(ASSIMP_BUILD_FBX_IMPORTER ON)
set(ASSIMP_BUILD_GLTF_IMPORTER ON)
set(ASSIMP_BUILD_COLLADA_IMPORTER ON)
findOrFetchPackage(https://github.com/assimp/assimp.git v6.*.* assimp 6.0.0...<7.0.0)

find_package(ZLIB REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

set(SHARED_INCLUDES
        "../lib/imgui"
        "../lib/imgui/backends"
        "../shared"
        ${OPENGL_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIRS}
)

set(SHARED_LIBRARIES
        SDL3::SDL3
        nlohmann_json::nlohmann_json
        miniaudio
        assets
        glm::glm
        assimp::assimp
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
        ZLIB::ZLIB
        glslang::glslang
        Vulkan::Vulkan
)

if (WIN32)
    set(SHARED_LIBRARIES
            ${SHARED_LIBRARIES}
            opengl32
    )
endif ()

add_custom_target(
        copy_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/bin/assets
)

add_subdirectory(libassets)
add_subdirectory(texedit)
add_subdirectory(mdledit)
add_subdirectory(sndedit)
add_subdirectory(lvledit)
add_subdirectory(shdedit)
add_subdirectory(fonedit)
