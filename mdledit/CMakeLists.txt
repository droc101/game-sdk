cmake_minimum_required(VERSION 3.20)
project(mdledit)

set(CMAKE_CXX_STANDARD 20)

add_executable(mdledit mdledit.cpp
        tabs/SkinsTab.cpp
        tabs/SkinsTab.h
        tabs/LodsTab.cpp
        tabs/LodsTab.h
        tabs/MaterialsTab.cpp
        tabs/MaterialsTab.h
        tabs/CollisionTab.cpp
        tabs/CollisionTab.h
        tabs/PreviewOptionsTab.cpp
        tabs/PreviewOptionsTab.h
        ModelEditor.cpp
        ModelEditor.h
        MdleditWindow.cpp
        MdleditWindow.h
)

configure_sdk_gui_program(mdledit)
