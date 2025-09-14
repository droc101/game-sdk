cmake_minimum_required(VERSION 3.20)
project(mdledit)

set(CMAKE_CXX_STANDARD 20)

add_executable(mdledit mdledit.cpp
        ModelRenderer.cpp
        ModelRenderer.h
        SkinEditWindow.cpp
        SkinEditWindow.h
        LodEditWindow.cpp
        LodEditWindow.h
        MaterialEditWindow.cpp
        MaterialEditWindow.h
        CollisionEditWindow.cpp
        CollisionEditWindow.h
)

set_target_properties(mdledit PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(mdledit PRIVATE shared imgui_opengl3)

add_dependencies(mdledit assets)

add_dependencies(mdledit copy_assets)

if (WIN32)
    add_dependencies(mdledit copydlls)
endif()
