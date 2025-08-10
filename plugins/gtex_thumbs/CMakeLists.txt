cmake_minimum_required(VERSION 3.20)
project(gtex_thumbs)

set(CMAKE_CXX_STANDARD 20)

set(BUILD_WITH_QT6 ON)
set(KDE_INSTALL_USE_QT_SYS_PATHS ON)
set(QT_MIN_VERSION "6.5.0")
set(KF_MIN_VERSION "6.0.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core Gui)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS KIO Config)

kcoreaddons_add_plugin(gtex_thumbs INSTALL_NAMESPACE "kf6/thumbcreator")

target_sources(gtex_thumbs PRIVATE
        gtex_thumbs.cpp
        gtex_thumbs.h
        gtex_thumbs.json
        dev.droc101.gtex_thumbs.metainfo.xml
)

add_dependencies(gtex_thumbs assets)

set_target_properties(gtex_thumbs PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(gtex_thumbs PRIVATE Qt6::Core Qt6::Gui KF6::KIOCore KF6::KIOGui assets)

install(FILES dev.droc101.gtex_thumbs.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES game-mime-types.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
