//
// Created by droc101 on 3/11/26.
//

#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <glm/glm.hpp>
#include <libassets/type/Actor.h>
#include <string>
#include <type_traits>

class Light // NOLINT(*-pro-type-member-init)
{
public:
    /// @warning Changes to this enum must be mirrored in the shaders and in the engine
    enum class Type : uint32_t // NOLINT(*-enum-size)
    {
        POINT,
        SPOT,
        AREA,
        DIRECTIONAL,
    };

    Light() = default;

    explicit Light(const Actor& actor);

    void Write(DataWriter& writer) const;

    Type type;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 negativeForwardDirection;
    glm::vec3 color;
    float brightness;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    float attenuationMultiplier;
    float brightAngle;
    float fadingAngle;
    std::string cookie;
};

// This is a requirement for Light to be used with the shaders
static_assert(std::is_standard_layout_v<Light> && sizeof(std::string) == 32);
