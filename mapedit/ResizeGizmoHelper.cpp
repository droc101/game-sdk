//
// Created by droc101 on 10/26/26.
//

#include "ResizeGizmoHelper.h"
#include <array>
#include <cstdint>
#include <libassets/type/Axis.h>
#include <libassets/type/BoundingBox.h>
#include <numbers>
#include <utility>
#include "Viewport.h"

ResizeGizmoHelper::Handle ResizeGizmoHelper::HoveredHandle(const Viewport &viewport, const BoundingBox &boundingBox)
{
    const std::array<glm::vec3, 8> &points = boundingBox.GetPoints();
    for (uint32_t i = 0; i < points.size(); i++)
    {
        if (glm::distance(viewport.GetLocalMousePos(), viewport.WorldToScreenPos(points.at(i))) <
            POINT_SIZE * std::numbers::sqrt2_v<float>)
        {
            const Handle handle = GetHandleFromBoundingBoxPointIndex(i, viewport.GetAxis());
            if (handle != Handle::NONE)
            {
                return handle;
            }
        }
    }
    for (uint32_t i = 0; i < LINES.size(); i++)
    {
        const std::pair<uint32_t, uint32_t> &line = LINES.at(i);
        const glm::vec3 &point = (points.at(line.second) - points.at(line.first)) / 2.0f + points.at(line.first);
        if (glm::distance(viewport.GetLocalMousePos(), viewport.WorldToScreenPos(point)) <
            POINT_SIZE * std::numbers::sqrt2_v<float>)
        {
            const Handle handle = GetHandleFromLineIndex(i, viewport.GetAxis());
            if (handle != Handle::NONE)
            {
                return handle;
            }
        }
    }
    return Handle::NONE;
}

void ResizeGizmoHelper::ScaleVertex(glm::vec3 &vertex,
                                    const BoundingBox &startBounds,
                                    const glm::vec3 &worldDrag,
                                    const Handle draggedHandle,
                                    const Axis axis)
{
    const glm::vec3 direction = HandleDirection(draggedHandle, axis);
    const glm::vec3 anchor = startBounds.origin - direction * startBounds.extents;
    const glm::vec3 scale = 1.0f + direction * worldDrag / glm::max(2.0f * startBounds.extents, glm::vec3{0.0001f});
    vertex = anchor + (vertex - anchor) * scale;
}

ResizeGizmoHelper::Handle ResizeGizmoHelper::GetHandleFromBoundingBoxPointIndex(const uint32_t index, const Axis axis)
{
    switch (axis)
    {
        case Axis::X:
            switch (index)
            {
                case 0:
                    return Handle::BOTTOM_RIGHT;
                case 1:
                    return Handle::BOTTOM_LEFT;
                case 2:
                    return Handle::TOP_RIGHT;
                case 3:
                    return Handle::TOP_LEFT;
                default:
                    return Handle::NONE;
            }
        case Axis::Y:
            switch (index)
            {
                case 5:
                    return Handle::BOTTOM_RIGHT;
                case 1:
                    return Handle::BOTTOM_LEFT;
                case 4:
                    return Handle::TOP_RIGHT;
                case 0:
                    return Handle::TOP_LEFT;
                default:
                    return Handle::NONE;
            }
        case Axis::Z:
            switch (index)
            {
                case 4:
                    return Handle::BOTTOM_RIGHT;
                case 0:
                    return Handle::BOTTOM_LEFT;
                case 6:
                    return Handle::TOP_RIGHT;
                case 2:
                    return Handle::TOP_LEFT;
                default:
                    return Handle::NONE;
            }
        default:
            return Handle::NONE;
    }
}

ResizeGizmoHelper::Handle ResizeGizmoHelper::GetHandleFromLineIndex(const uint32_t index, const Axis axis)
{
    switch (axis)
    {
        case Axis::X:
            switch (index)
            {
                case 2:
                    return Handle::TOP_CENTER;
                case 0:
                    return Handle::BOTTOM_CENTER;
                case 7:
                    return Handle::LEFT_CENTER;
                case 1:
                    return Handle::RIGHT_CENTER;
                default:
                    return Handle::NONE;
            }
        case Axis::Y:
            switch (index)
            {
                case 3:
                    return Handle::TOP_CENTER;
                case 4:
                    return Handle::BOTTOM_CENTER;
                case 0:
                    return Handle::LEFT_CENTER;
                case 5:
                    return Handle::RIGHT_CENTER;
                default:
                    return Handle::NONE;
            }
        case Axis::Z:
            switch (index)
            {
                case 6:
                    return Handle::TOP_CENTER;
                case 3:
                    return Handle::BOTTOM_CENTER;
                case 1:
                    return Handle::LEFT_CENTER;
                case 7:
                    return Handle::RIGHT_CENTER;
                default:
                    return Handle::NONE;
            }
        default:
            return Handle::NONE;
    }
}

glm::vec3 ResizeGizmoHelper::HandleDirection(const Handle draggedHandle, const Axis axis)
{
    switch (axis)
    {
        case Axis::X:
            switch (draggedHandle)
            {
                case Handle::TOP_LEFT:
                    return glm::vec3{0, 1, 1};
                case Handle::TOP_RIGHT:
                    return glm::vec3{0, 1, -1};
                case Handle::BOTTOM_LEFT:
                    return glm::vec3{0, -1, 1};
                case Handle::BOTTOM_RIGHT:
                    return glm::vec3{0, -1, -1};
                case Handle::TOP_CENTER:
                    return glm::vec3{0, 1, 0};
                case Handle::BOTTOM_CENTER:
                    return glm::vec3{0, -1, 0};
                case Handle::LEFT_CENTER:
                    return glm::vec3{0, 0, 1};
                case Handle::RIGHT_CENTER:
                    return glm::vec3{0, 0, -1};
                default:
                    return glm::vec3{0};
            }
        case Axis::Y:
            switch (draggedHandle)
            {
                case Handle::TOP_LEFT:
                    return glm::vec3{-1, 0, -1};
                case Handle::TOP_RIGHT:
                    return glm::vec3{1, 0, -1};
                case Handle::BOTTOM_LEFT:
                    return glm::vec3{-1, 0, 1};
                case Handle::BOTTOM_RIGHT:
                    return glm::vec3{1, 0, 1};
                case Handle::TOP_CENTER:
                    return glm::vec3{0, 0, -1};
                case Handle::BOTTOM_CENTER:
                    return glm::vec3{0, 0, 1};
                case Handle::LEFT_CENTER:
                    return glm::vec3{-1, 0, 0};
                case Handle::RIGHT_CENTER:
                    return glm::vec3{1, 0, 0};
                default:
                    return glm::vec3{0};
            }
        case Axis::Z:
            switch (draggedHandle)
            {
                case Handle::TOP_LEFT:
                    return glm::vec3{-1, 1, 0};
                case Handle::TOP_RIGHT:
                    return glm::vec3{1, 1, 0};
                case Handle::BOTTOM_LEFT:
                    return glm::vec3{-1, -1, 0};
                case Handle::BOTTOM_RIGHT:
                    return glm::vec3{1, -1, 0};
                case Handle::TOP_CENTER:
                    return glm::vec3{0, 1, 0};
                case Handle::BOTTOM_CENTER:
                    return glm::vec3{0, -1, 0};
                case Handle::LEFT_CENTER:
                    return glm::vec3{-1, 0, 0};
                case Handle::RIGHT_CENTER:
                    return glm::vec3{1, 0, 0};
                default:
                    return glm::vec3{0};
            }
        default:
            return glm::vec3{0};
    }
}
