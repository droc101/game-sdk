//
// Created by droc101 on 9/2/26.
//

#ifndef GAME_SDK_AXIS_H
#define GAME_SDK_AXIS_H

#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

enum class Axis : uint8_t
{
    X,
    Y,
    Z,
};

class AxisHelper
{
    public:
        [[nodiscard]] static glm::vec3 Make3D(Axis axis, glm::vec2 twoDimensionalComponent, float otherAxis);

        [[nodiscard]] static glm::vec2 Make2D(Axis axis, glm::vec3 point);

        [[nodiscard]] static float GetComponent(Axis axis, glm::vec3 vector);

        static void SetComponent(Axis axis, glm::vec3 &vec, float component);

        static void Set2DComponents(Axis ignoredAxis, glm::vec3 &dest, glm::vec2 twoDimensionalComponents);
};

#endif //GAME_SDK_AXIS_H
