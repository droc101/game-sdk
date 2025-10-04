//
// Created by droc101 on 10/3/25.
//

#pragma once

#include <cstddef>
#include <libassets/util/Color.h>
#include <libassets/util/Sector.h>
#include <libassets/util/WallMaterial.h>
#include "../Viewport.h"
#include "EditorTool.h"

class SelectTool final: public EditorTool
{
    public:
        SelectTool() = default;
        ~SelectTool() override = default;

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
                                glm::vec3 startCeiling,
                                Color &lineColor);

        void RenderViewportSelectMode(const ::Viewport &vp,
                                      glm::mat4 &matrix,
                                      bool isHovered,
                                      const glm::vec3 &worldSpaceHover);

        void RenderViewportVertexMode(Viewport &vp,
                                      glm::mat4 &matrix,
                                      bool isHovered,
                                      glm::vec3 &worldSpaceHover,
                                      glm::vec2 &screenSpaceHover);

        void RenderViewport(Viewport &vp) override;

        void RenderToolWindow() override;

    private:
        bool sectorFocusMode = false;
        size_t focusedSectorIndex = 0;

        ItemType hoverType = ItemType::NONE;
        size_t hoverIndex = 0;

        ItemType selectionType = ItemType::NONE;
        size_t selectionIndex = 0;
        size_t selectionVertexIndex = 0;

        /// The difference from the first vertex to the 2nd vertex (firstVertex + lineDragModeSecondVertexOffset = secondVertex)
        glm::vec2 lineDragModeSecondVertexOffset{};
        /// The difference from the mouse to the 1st vertex (worldSpaceMouse - lineDragModeMouseOffset = firstVertex)
        glm::vec2 lineDragModeMouseOffset{};

        bool dragging = false;

        static void MaterialToolWindow(WallMaterial &wallMat);
};
