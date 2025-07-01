cmake_minimum_required(VERSION 3.20)
project(mdledit)

set(CMAKE_CXX_STANDARD 20)

add_executable(mdledit main.cpp
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
        ../lib/imgui/backends/imgui_impl_opengl3.h
        ../lib/imgui/backends/imgui_impl_opengl3.cpp
        ../lib/imgui/backends/imgui_impl_opengl3_loader.h
        ../lib/imgui/backends/imgui_impl_sdl3.cpp
        ../lib/imgui/backends/imgui_impl_sdl3.h
        ModelRenderer.cpp
        ModelRenderer.h
        ../shared/Options.cpp
        ../shared/Options.h
        ../shared/OptionsWindow.cpp
        ../shared/OptionsWindow.h
        ../shared/AboutWindow.cpp
        ../shared/AboutWindow.h
        ../shared/SharedMgr.cpp
        ../shared/SharedMgr.h
        SkinEditWindow.cpp
        SkinEditWindow.h)

find_package(SDL3 REQUIRED)
include_directories(${SDL3_INCLUDE_DIR})

include_directories("../lib/imgui")
include_directories("../lib/imgui/backends")

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

find_package(glm REQUIRED)
include_directories(${GLM_INCLUDE_DIRS})

include_directories("../lib/json/include")

find_package(assimp REQUIRED)
include_directories(${ASSIMP_INCLUDE_DIRS})

find_package(OpenGL REQUIRED) # Graphics
include_directories(${OPENGL_INCLUDE_DIR})

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

include_directories("../libassets/include")
include_directories("../shared")

set_target_properties(mdledit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(mdledit PRIVATE ${SDL3_LIBRARY} SDL3 z GL GLU glm::glm nlohmann_json::nlohmann_json assimp ${OPENGL_gl_LIBRARY} ${GLEW_LIBRARIES} ${CMAKE_BINARY_DIR}/libassets/libassets.a)

add_dependencies(mdledit assets)

add_custom_target(
        copy_mdledit_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/mdledit/assets ${CMAKE_BINARY_DIR}/mdledit/assets
)
add_dependencies(mdledit copy_mdledit_assets)
