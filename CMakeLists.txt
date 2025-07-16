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

    foreach(dll IN LISTS DLLS)
        add_custom_command(TARGET copydlls POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_PATH}\\bin\\${dll}"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\\${dll}"
        )
    endforeach()
endif()

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
)

if (WIN32)
    set(SHARED_SRC
        ${SHARED_SRC}
        ../game-sdk.rc
    )
endif ()

set(MINIAUDIO_SRC
        ../lib/miniaudio/miniaudio.h
        ../lib/miniaudio/miniaudio.c
)

find_package(SDL3 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

set(SHARED_INCLUDES
        "../lib/imgui"
        "../lib/imgui/backends"
        "../libassets/include"
        ${SDL3_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIRS}
        "../lib/json/include"
        "../shared"
        ${GLM_INCLUDE_DIRS}
        ${ASSIMP_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIRS}
)

set(SHARED_LIBRARIES
        ${SDL3_LIBRARIES}
        ${ZLIB_LIBRARIES}
        nlohmann_json::nlohmann_json
        ${CMAKE_BINARY_DIR}/libassets/libassets.a
        glm::glm # TODO find variables for glm and assimp
        assimp::assimp # ${assimp_LIBRARIES} doesn't work
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
        opengl32
)

add_subdirectory(lib/json)
add_subdirectory(libassets)
add_subdirectory(texedit)
add_subdirectory(mdledit)
add_subdirectory(sndedit)
