//
// Created by droc101 on 3/11/26.
//

#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <libassets/type/Actor.h>
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
        explicit Light(const Actor &actor)
        {
            position = actor.position;
            forwardDirection = glm::normalize(glm::vec3{
                -sin(glm::radians(actor.rotation.y)) * cos(glm::radians(actor.rotation.x)),
                sin(glm::radians(actor.rotation.x)),
                -cos(glm::radians(actor.rotation.y)) * cos(glm::radians(actor.rotation.x)),
            });
            const float *colorPtr = actor.params.at("color").Get<Color>(Color(-1u)).GetDataPointer();
            color = glm::vec3{colorPtr[0], colorPtr[1], colorPtr[2]};
            brightness = actor.params.at("brightness").Get<float>(1.0f);

            if (actor.className == "light_directional")
            {
                type = Type::DIRECTIONAL;
            } else
            {
                constantAttenuation = actor.params.at("constant_attenuation").Get<float>(0.0f);
                linearAttenuation = actor.params.at("linear_attenuation").Get<float>(0.0f);
                quadraticAttenuation = actor.params.at("quadratic_attenuation").Get<float>(1.0f);
                attenuationMultiplier = actor.params.at("attenuation_multiplier").Get<float>(2.0f);
                if (actor.className == "light_point")
                {
                    type = Type::POINT;
                } else if (actor.className == "light_spot")
                {
                    type = Type::SPOT;
                    brightAngle = actor.params.at("bright_angle").Get<float>(30.0f);
                    fadingAngle = actor.params.at("fading_angle").Get<float>(45.0f);
                }
            }
        }
        // NOLINTEND(*-pro-type-member-init)
        // ReSharper restore CppPossiblyUninitializedMember

        alignas(4) Type type;
        alignas(16) glm::vec3 position;
        alignas(16) glm::vec3 forwardDirection;
        alignas(16) glm::vec3 color;
        alignas(4) float brightness;
        alignas(4) float constantAttenuation;
        alignas(4) float linearAttenuation;
        alignas(4) float quadraticAttenuation;
        alignas(4) float attenuationMultiplier;
        alignas(4) float brightAngle;
        alignas(4) float fadingAngle;
};

// This is a requirement for Light to be considered a trivial type, and this is not true if the members have a
//  default initialization
static_assert(std::is_trivially_default_constructible_v<Light>);
