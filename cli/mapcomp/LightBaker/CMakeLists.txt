set(CMAKE_CXX_STANDARD 23)

FetchContent_Declare(
        Luna
        GIT_REPOSITORY https://github.com/NBT22/Luna
        GIT_TAG b374cb2f6c479abe02dd4a882da7789528248e44
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(Luna)

add_library(LightBaker INTERFACE)

target_sources(LightBaker INTERFACE
        include/AccelerationStructure.hpp
        src/AccelerationStructure.cpp
        include/VulkanContext.hpp
        src/VulkanContext.cpp
        include/Lightmap.hpp
        src/Lightmap.cpp
        include/ShaderLight.hpp
        src/ShaderLight.cpp
        include/LightBaker.hpp
        src/LightBaker.cpp
        include/RenderdocManager.h
        src/RenderdocManager.cpp
        include/NsightManager.h
        src/NsightManager.cpp
)

target_include_directories(LightBaker INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../" "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(LightBaker INTERFACE Luna)
