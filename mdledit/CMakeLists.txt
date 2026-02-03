cmake_minimum_required(VERSION 3.20)
project(mdledit)

set(CMAKE_CXX_STANDARD 20)

add_executable(mdledit mdledit.cpp
        ModelRenderer.cpp
        ModelRenderer.h
        tabs/SkinsTab.cpp
        tabs/SkinsTab.h
        tabs/LodsTab.cpp
        tabs/LodsTab.h
        tabs/MaterialsTab.cpp
        tabs/MaterialsTab.h
        tabs/CollisionTab.cpp
        tabs/CollisionTab.h
        $<$<BOOL:${WIN32}>:mdledit.rc>
        tabs/PreviewOptionsTab.cpp
        tabs/PreviewOptionsTab.h
)

set_target_properties(mdledit PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "-Wl,-rpath='$ORIGIN'" PREFIX "")

target_link_libraries(mdledit PRIVATE shared)

add_dependencies(mdledit assets)

add_dependencies(mdledit copy_assets)

if (WIN32)
    add_dependencies(mdledit copydlls)
    target_sources(mdledit PRIVATE ../app.manifest)
endif()
