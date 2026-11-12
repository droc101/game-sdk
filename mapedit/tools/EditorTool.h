//
// Created by droc101 on 9/7/25.
//

#pragma once

#include <cstdint>

class Viewport;

class EditorTool
{
    public:
        EditorTool() = default;
        virtual ~EditorTool() = default;

        virtual void RenderViewport(Viewport &vp) = 0;

        virtual void RenderToolWindow() = 0;

        enum class ItemType : uint8_t
        {
            NONE,
            VERTEX,
            LINE,
            BRUSH,
            ACTOR,
            FACE,
        };
        static_assert(ItemType::NONE == static_cast<ItemType>(0));
};
