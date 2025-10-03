//
// Created by droc101 on 9/7/25.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <libassets/util/Sector.h>
#include "../Viewport.h"
#include "EditorTool.h"

/**
 * This tool is used to edit the vertices & height of sectors.
 */
class VertexTool final: public EditorTool
{
    public:
        VertexTool() = default;
        VertexTool(size_t sectorIndex);
        ~VertexTool() override = default;

        void RenderViewport(Viewport &vp) override;

        void RenderToolWindow() override;

        void HandleDrag(const Viewport &vp, bool isHovered, glm::vec3 worldSpaceHover);

        void ProcessSectorHover(const Viewport &vp,
                                const Sector &sector,
                                bool isHovered,
                                glm::vec2 screenSpaceHover,
                                size_t sectorIndex);

        void ProcessVertexHover(const Viewport &viewport,
                                glm::vec2 vertexScreenSpace,
                                glm::vec2 screenSpaceHover,
                                bool isHovered,
                                Sector &sector,
                                glm::vec2 endVertexScreenSpace,
                                glm::vec3 worldSpaceHover,
                                size_t vertexIndex,
                                size_t sectorIndex,
                                Color &vertexColor,
                                glm::vec3 startCeiling);

    private:
        ItemType selectionType = ItemType::NONE;
        size_t selectionSectorIndex = 0;
        size_t selectionVertexIndex = 0;
        /// The difference from the first vertex to the 2nd vertex (firstVertex + lineDragModeSecondVertexOffset = secondVertex)
        glm::vec2 lineDragModeSecondVertexOffset{};
        /// The difference from the mouse to the 1st vertex (worldSpaceMouse - lineDragModeMouseOffset = firstVertex)
        glm::vec2 lineDragModeMouseOffset{};

        bool sectorLock = false;
        size_t sectorLockIndex = 0;
};
