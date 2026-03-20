//
// Created by NBT22 on 3/14/26.
//

#pragma once

#include <glm/vec3.hpp>
#include <libassets/util/DataWriter.h>
#include <type_traits>

class MapVertex
{
    public:
        void Write(DataWriter &writer) const;

        alignas(16) glm::vec3 position;

        alignas(8) glm::vec2 uv;

        alignas(8) glm::vec2 lightmapUv;

        /// The normal of the surface this point is on
        /// @note Only used by lightmap compiling. Not actually written to the asset binary.
        alignas(16) glm::vec3 normal;
};

// This is a requirement for MapVertex to be considered a trivial type, and this is not true if the members have a
//  default initialization
static_assert(std::is_trivially_default_constructible_v<MapVertex>);
