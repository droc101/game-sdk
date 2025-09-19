//
// Created by droc101 on 9/7/25.
//

#pragma once
#include "../Viewport.h"
#include "EditorTool.h"
#include <cstddef>
#include <cstdint>

/**
 * This tool is used to edit the vertices & height of sectors.
 */
class VertexTool final: public EditorTool
{
    public:
        VertexTool() = default;
        ~VertexTool() override = default;

        void RenderViewport(Viewport &vp) override;

        void RenderToolWindow() override;

        void HandleDrag(const Viewport &vp, bool isHovered, glm::vec3 worldSpaceHover);

        void ProcessSectorHover(const Viewport &vp, const Sector &sector, bool isHovered,
                                glm::vec2 screenSpaceHover, size_t s);

        void ProcessVertexHover(const Viewport &vp, glm::vec2 vertexScreenSpace, glm::vec2 screenSpaceHover,
                                bool isHovered, Sector &sector, glm::vec2 endVertexScreenSpace,
                                glm::vec3 worldSpaceHover, size_t i, size_t s,
                                Color &c, glm::vec3 start_ceiling);

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
        /// The difference from the first vertex to the 2nd vertex (firstVertex + lineDragModeSecondVertexOffset = secondVertex)
        glm::vec2 lineDragModeSecondVertexOffset{};
        /// The difference from the mouse to the 1st vertex (worldSpaceMouse - lineDragModeMouseOffset = firstVertex)
        glm::vec2 lineDragModeMouseOffset{};

};
