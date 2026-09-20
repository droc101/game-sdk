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

        // ReSharper disable CppPossiblyUninitializedMember
        // NOLINTBEGIN(*-pro-type-member-init)
        Light() = default;

        // NOLINTEND(*-pro-type-member-init)
        explicit Light(const Actor &actor)
        {
            position = actor.position;
            rotation = actor.rotation;
            negativeForwardDirection = glm::normalize(glm::vec3{
                    sin(glm::radians(actor.rotation.y)) * cos(glm::radians(actor.rotation.x)),
                    -sin(glm::radians(actor.rotation.x)),
                    cos(glm::radians(actor.rotation.y)) * cos(glm::radians(actor.rotation.x)),
            });
            const Color c = Param::KvListGet(actor.params, "color", Color(-1));
            color = glm::vec3{c.R(), c.G(), c.B()};
            brightness = Param::KvListGet(actor.params, "brightness", 1.0f);

            if (actor.className == "light_directional")
            {
                type = Type::DIRECTIONAL;
            } else
            {
                constantAttenuation = Param::KvListGet(actor.params, "constant_attenuation", 0.0f);
                linearAttenuation = Param::KvListGet(actor.params, "linear_attenuation", 0.0f);
                quadraticAttenuation = Param::KvListGet(actor.params, "quadratic_attenuation", 1.0f);
                attenuationMultiplier = Param::KvListGet(actor.params, "attenuation_multiplier", 2.0f);
                if (actor.className == "light_point")
                {
                    type = Type::POINT;
                } else if (actor.className == "light_spot")
                {
                    type = Type::SPOT;
                    brightAngle = Param::KvListGet(actor.params, "bright_angle", 30.0f);
                    fadingAngle = Param::KvListGet(actor.params, "fading_angle", 45.0f);
                    cookie = Param::KvListGet(actor.params, "cookie", std::string(""));
                }
            }
        }

        // ReSharper restore CppPossiblyUninitializedMember

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
