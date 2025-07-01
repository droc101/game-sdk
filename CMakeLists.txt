cmake_minimum_required(VERSION 3.20)
project(game-sdk)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Wpedantic -Wall -Wextra -Wno-missing-field-initializers")

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DBUILDSTYLE_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-g0 -O3 -s -DNDEBUG -DBUILDSTYLE_RELEASE")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O3 -DBUILDSTYLE_DEBUG")

add_subdirectory(lib/json)
add_subdirectory(launcher)
add_subdirectory(libassets)
add_subdirectory(texedit)
add_subdirectory(tex)
add_subdirectory(mdledit)