//
// Created by droc101 on 10/26/26.
//

#pragma once

#include <array>
#include <cstdint>
#include <libassets/type/Axis.h>
#include <libassets/type/BoundingBox.h>
#include <utility>
#include "Viewport.h"

class ResizeGizmoHelper
{
    public:
        enum class Handle : uint8_t
        {
            NONE,
            TOP_LEFT,
            TOP_RIGHT,
            BOTTOM_LEFT,
            BOTTOM_RIGHT,
            TOP_CENTER,
            BOTTOM_CENTER,
            LEFT_CENTER,
            RIGHT_CENTER,
        };

        ResizeGizmoHelper() = delete;

        [[nodiscard]] static Handle HoveredHandle(const Viewport &viewport, const BoundingBox &boundingBox);

        static void ScaleVertex(glm::vec3 &vertex,
                                const BoundingBox &startBounds,
                                const glm::vec3 &worldDrag,
                                Handle draggedHandle,
                                Axis axis);

        static constexpr float POINT_SIZE = 10;
        static constexpr float LINE_THICKNESS = 2;

        static constexpr std::array<std::pair<uint32_t, uint32_t>, 8> LINES = {
            std::pair{0, 1},
            std::pair{0, 2},
            std::pair{2, 3},
            std::pair{0, 4},
            std::pair{1, 5},
            std::pair{4, 5},
            std::pair{2, 6},
            std::pair{5, 7},
        };

    private:
        [[nodiscard]] static Handle GetHandleFromBoundingBoxPointIndex(uint32_t index, Axis axis);

        [[nodiscard]] static Handle GetHandleFromLineIndex(uint32_t index, Axis axis);

        /**
         * Get which side of the bounding box a handle is on, per axis
         * @return -1 for the min side, 1 for the max side, or 0 if the handle doesn't scale that axis
         */
        [[nodiscard]] static glm::vec3 HandleDirection(Handle draggedHandle, Axis axis);
};
