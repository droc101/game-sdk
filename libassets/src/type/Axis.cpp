//
// Created by droc101 on 9/3/26.
//

#include <libassets/type/Axis.h>

glm::vec3 AxisHelper::Make3D(const Axis axis, glm::vec2 twoDimensionalComponent, float otherAxis)
{
    switch (axis)
    {
        case Axis::Y:
            return {twoDimensionalComponent.x, otherAxis, twoDimensionalComponent.y};
        case Axis::Z:
            return {twoDimensionalComponent.x, twoDimensionalComponent.y, otherAxis};
        case Axis::X:
            return {otherAxis, twoDimensionalComponent.y, twoDimensionalComponent.x};
    }
    return glm::vec3(0);
}

glm::vec2 AxisHelper::Make2D(const Axis axis, glm::vec3 point)
{
    switch (axis)
    {
        case Axis::Y:
            return {point.x, point.z};
        case Axis::Z:
            return {point.x, point.y};
        case Axis::X:
            return {point.z, point.y};
    }
    return glm::vec3(0);
}

float AxisHelper::GetComponent(const Axis axis, const glm::vec3 vector)
{
    switch (axis)
    {
        case Axis::Y:
            return vector.y;
        case Axis::Z:
            return vector.z;
        case Axis::X:
            return vector.x;
    }
    return 0;
}

void AxisHelper::SetComponent(const Axis axis, glm::vec3 &vec, const float component)
{
    switch (axis)
    {
        case Axis::Y:
            vec.y = component;
            break;
        case Axis::Z:
            vec.z = component;
            break;
        case Axis::X:
            vec.x = component;
            break;
    }
}
