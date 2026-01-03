include(FetchContent)

macro(getLatestPackageVersion gitRepo versionSplat)
    find_package(Git 2.18 REQUIRED)
    if (WIN32)
        execute_process(COMMAND powershell -command "((& '${GIT_EXECUTABLE}' -c 'versionsort.suffix=-' ls-remote --exit-code --refs --sort=version:refname --tags ${gitRepo} '${versionSplat}' | Select-Object -Last 1) -Split '/')[2]" OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE LATEST_RELEASE)
    else ()
        execute_process(COMMAND ${GIT_EXECUTABLE} -c "versionsort.suffix=-" ls-remote --exit-code --refs --sort=version:refname --tags ${gitRepo} "${versionSplat}" COMMAND tail --lines=1 COMMAND cut --delimiter=/ --fields=3 COMMAND tr -d "\n" OUTPUT_VARIABLE LATEST_RELEASE)
    endif ()
endmacro()

macro(findOrFetchPackage gitRepo versionSplat packageName)
    getLatestPackageVersion(${gitRepo} ${versionSplat})

    FetchContent_Declare(
            ${packageName}
            GIT_REPOSITORY ${gitRepo}
            GIT_TAG ${LATEST_RELEASE}
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
            EXCLUDE_FROM_ALL
            SYSTEM
            FIND_PACKAGE_ARGS ${ARGN}
    )
    FetchContent_MakeAvailable(${packageName})
endmacro()

macro(fetchPackage gitRepo versionSplat packageName)
    getLatestPackageVersion(${gitRepo} ${versionSplat})

    FetchContent_Declare(
            ${packageName}
            GIT_REPOSITORY ${gitRepo}
            GIT_TAG ${LATEST_RELEASE}
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
            EXCLUDE_FROM_ALL
            SYSTEM
    )
    FetchContent_MakeAvailable(${packageName})
endmacro()

macro(get_latest_package_version git_repo version_glob)
    find_package(Git 2.18 REQUIRED)
    if (WIN32)
        execute_process(COMMAND powershell -command "((& '${GIT_EXECUTABLE}' -c 'versionsort.suffix=-' ls-remote --exit-code --refs --sort=version:refname --tags ${git_repo} '${version_glob}' | Select-Object -Last 1) -Split '/')[2]" OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE LATEST_RELEASE)
    else ()
        execute_process(COMMAND ${GIT_EXECUTABLE} -c "versionsort.suffix=-" ls-remote --exit-code --refs --sort=version:refname --tags ${git_repo} "${version_glob}" COMMAND tail --lines=1 COMMAND cut --delimiter=/ --fields=3 COMMAND tr -d "\n" OUTPUT_VARIABLE LATEST_RELEASE)
    endif ()
endmacro()

function(fetch_earcut_hpp)
    set(EARCUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/earcut)
    get_latest_package_version(https://github.com/mapbox/earcut.hpp.git v2.*.*)
    file(DOWNLOAD https://raw.githubusercontent.com/mapbox/earcut.hpp/refs/tags/${LATEST_RELEASE}/include/mapbox/earcut.hpp ${EARCUT_DIR}/include/mapbox/earcut.hpp)
    file(DOWNLOAD https://raw.githubusercontent.com/mapbox/earcut.hpp/refs/tags/${LATEST_RELEASE}/LICENSE ${EARCUT_DIR}/LICENSE)

    add_library(earcut INTERFACE)
    target_include_directories(earcut INTERFACE ${EARCUT_DIR}/include)
endfunction()

function(fetch_glew)
    set(GLEW_EGL ON)
    set(GLEW_GLX ON)
    set(BUILD_UTILS OFF)
    set(BUILD_SHARED_LIBS OFF)

    getLatestPackageVersion(https://github.com/nigels-com/glew glew-2.3.*)

    FetchContent_Declare(
            GLEW
            URL https://github.com/nigels-com/glew/releases/download/${LATEST_RELEASE}/${LATEST_RELEASE}.tgz
            SOURCE_SUBDIR "build/cmake"
            DOWNLOAD_EXTRACT_TIMESTAMP
            EXCLUDE_FROM_ALL
            SYSTEM
    )
    FetchContent_MakeAvailable(GLEW)
endfunction()
