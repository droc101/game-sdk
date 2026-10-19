include(FetchContent)

macro(get_latest_package_version git_repo version_glob)
    find_package(Git 2.18 REQUIRED)
    if (WIN32)
        execute_process(COMMAND powershell -command "((& '${GIT_EXECUTABLE}' -c 'versionsort.suffix=-' ls-remote --exit-code --refs --sort=version:refname --tags ${git_repo} '${version_glob}' | Select-Object -Last 1) -Split '/')[2]" OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE LATEST_RELEASE)
    else ()
        execute_process(COMMAND ${GIT_EXECUTABLE} -c "versionsort.suffix=-" ls-remote --exit-code --refs --sort=version:refname --tags ${git_repo} "${version_glob}" COMMAND tail -n 1 COMMAND cut -d / -f 3 COMMAND tr -d "\n" OUTPUT_VARIABLE LATEST_RELEASE)
    endif ()
endmacro()

macro(find_or_fetch_package gitRepo versionSplat packageName)
    get_latest_package_version(${gitRepo} ${versionSplat})

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

macro(fetch_package gitRepo versionSplat packageName)
    get_latest_package_version(${gitRepo} ${versionSplat})

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

function(fetch_glew)
    set(GLEW_EGL ON)
    set(GLEW_GLX ON)
    set(BUILD_UTILS OFF)
    set(BUILD_SHARED_LIBS OFF)

    get_latest_package_version(https://github.com/nigels-com/glew glew-2.3.*)

    FetchContent_Declare(
            GLEW
            URL https://github.com/nigels-com/glew/releases/download/${LATEST_RELEASE}/${LATEST_RELEASE}.tgz
            SOURCE_SUBDIR "build/cmake"
            DOWNLOAD_EXTRACT_TIMESTAMP
            EXCLUDE_FROM_ALL
            SYSTEM
    )
    FetchContent_MakeAvailable(GLEW)
    set_target_properties(glew_s PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
endfunction()

function(fetch_tinyexpr)
    set(TINYEXPR_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/tinyexpr)
    set(TINYEPXR_COMMIT "4a7456e2eab88b4c76053c1c4157639ccb930e2b")
    file(DOWNLOAD https://raw.githubusercontent.com/codeplea/tinyexpr/${TINYEPXR_COMMIT}/tinyexpr.c ${TINYEXPR_DIR}/tinyexpr.c)
    file(DOWNLOAD https://raw.githubusercontent.com/codeplea/tinyexpr/${TINYEPXR_COMMIT}/tinyexpr.h ${TINYEXPR_DIR}/tinyexpr.h)
    file(DOWNLOAD https://raw.githubusercontent.com/codeplea/tinyexpr/${TINYEPXR_COMMIT}/LICENSE ${TINYEXPR_DIR}/LICENSE)

    add_library(tinyexpr INTERFACE)
    target_sources(tinyexpr INTERFACE ${TINYEXPR_DIR}/tinyexpr.c ${TINYEXPR_DIR}/tinyexpr.h)
    target_include_directories(tinyexpr INTERFACE ${TINYEXPR_DIR})
endfunction()

