//
// Created by droc101 on 3/11/26.
//

#pragma once

#include <cstdint>
#include <type_traits>

class Light // NOLINT(*-pro-type-member-init)
{
    public:
        enum class Type : uint32_t // NOLINT(*-enum-size)
        {
            Point,
            Spot,
            Area,
            Directional,
        };

        // ReSharper disable CppPossiblyUninitializedMember
        // NOLINTBEGIN(*-pro-type-member-init)
        Light() = default;
        explicit Light(const glm::vec3 position,
                       const glm::vec3 color = glm::vec3(1, 1, 1),
                       const float brightnessScale = 1.0f,
                       const float range = 2.0f,
                       const float attenuation = 2.0f):
            type(Type::Point),
            position(position),
            color(color),
            brightnessScale(brightnessScale),
            range(range),
            attenuation(attenuation)
        {}
        explicit Light(const glm::vec3 position,
                       const glm::vec3 rotation,
                       const glm::vec3 color = glm::vec3(1, 1, 1),
                       const float brightnessScale = 1.0f,
                       const float range = 2.0f,
                       const float attenuation = 2.0f,
                       const float coneAngle = 45.0f,
                       const float angularAttenuation = 2.0f):
            type(Type::Spot),
            position(position),
            rotation(rotation),
            color(color),
            brightnessScale(brightnessScale),
            range(range),
            attenuation(attenuation),
            coneAngle(coneAngle),
            angularAttenuation(angularAttenuation)
        {}
        explicit Light(const glm::vec3 rotation,
                       const glm::vec3 color = glm::vec3(1, 1, 1),
                       const float brightnessScale = 1.0f):
            type(Type::Directional),
            rotation(rotation),
            color(color),
            brightnessScale(brightnessScale)
        {}
        explicit Light(const Actor &actor)
        {
            if (actor.className == "light_point")
            {
                type = Type::Point;
                position = actor.position;
                const float *colorPtr = actor.params.at("color").Get<Color>(Color(-1u)).GetDataPointer();
                color = glm::vec3{colorPtr[0], colorPtr[1], colorPtr[2]};
                brightnessScale = actor.params.at("brightness").Get<float>(1.0f);
                range = actor.params.at("range").Get<float>(2.0f);
                attenuation = actor.params.at("attenuation").Get<float>(2.0f);
            } else if (actor.className == "light_spot")
            {
                type = Type::Spot;
                position = actor.position;
                rotation = actor.rotation;
                const float *colorPtr = actor.params.at("color").Get<Color>(Color(-1u)).GetDataPointer();
                color = glm::vec3{colorPtr[0], colorPtr[1], colorPtr[2]};
                brightnessScale = actor.params.at("brightness").Get<float>(1.0f);
                range = actor.params.at("range").Get<float>(2.0f);
                attenuation = actor.params.at("attenuation").Get<float>(2.0f);
                coneAngle = actor.params.at("angle").Get<float>(45.0f);
                angularAttenuation = actor.params.at("angle_attenuation").Get<float>(2.0f);
            } else if (actor.className == "light_directional")
            {
                type = Type::Directional;
                rotation = actor.rotation;
                const float *colorPtr = actor.params.at("color").Get<Color>(Color(-1u)).GetDataPointer();
                color = glm::vec3{colorPtr[0], colorPtr[1], colorPtr[2]};
                brightnessScale = actor.params.at("brightness").Get<float>(1.0f);
            }
        }
        // NOLINTEND(*-pro-type-member-init)
        // ReSharper restore CppPossiblyUninitializedMember

        alignas(4) Type type;
        alignas(16) glm::vec3 position;
        alignas(16) glm::vec3 rotation;
        alignas(16) glm::vec3 color;
        alignas(4) float brightnessScale;
        alignas(4) float range;
        alignas(4) float attenuation;
        alignas(4) float coneAngle;
        alignas(4) float angularAttenuation;
};

// This is a requirement for Light to be considered a trivial type, and this is not true if the members have a
//  default initialization
static_assert(std::is_trivially_default_constructible_v<Light>);
