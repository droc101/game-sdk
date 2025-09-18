//
// Created by droc101 on 9/7/25.
//

#pragma once
#include "../Viewport.h"
#include "EditorTool.h"
#include <cstddef>

/**
 * This tool is used to edit the vertices & height of sectors.
 */
class VertexTool final : public EditorTool
{
    public:
        VertexTool() = default;
        ~VertexTool() override = default;

        void RenderViewport(Viewport &vp) override;

        void RenderToolWindow() override;

    private:
        enum class DragType: uint8_t
        {
            NONE,
            VERTEX,
            LINE, // TODO
            CEILING,
            FLOOR
        };

        DragType dragType = DragType::NONE;
        size_t sectorIndex = 0;
        size_t vertexIndex = 0;

};
