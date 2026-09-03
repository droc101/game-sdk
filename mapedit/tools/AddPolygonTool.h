//
// Created by droc101 on 9/19/25.
//

#pragma once

#include <cstdint>
#include <libassets/type/Axis.h>
#include <vector>
#include "../Viewport.h"
#include "EditorTool.h"

class AddPolygonTool final: public EditorTool
{
    public:
        AddPolygonTool() = default;
        ~AddPolygonTool() override = default;

        void RenderViewport(Viewport &vp) override;

        void RenderToolWindow() override;

    private:

        enum class PolygonToolState : uint8_t
        {
            IDLE,
            DRAWING,
            WAITING_TO_PLACE,
            DRAGGING_START_DEPTH,
            DRAGGING_END_DEPTH,
        };

        static inline PolygonToolState state = PolygonToolState::IDLE;
        static inline std::vector<glm::vec2> points{};
        static inline float startDepth = 16;
        static inline float endDepth = -16;
        static inline Axis axis = Axis::Y;
        static inline glm::vec2 shapeStart{};
        static inline glm::vec2 shapeEnd{};

        void AddBrush();
};
