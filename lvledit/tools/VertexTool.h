//
// Created by droc101 on 9/7/25.
//

#pragma once
#include "../Viewport.h"
#include "EditorTool.h"
#include <cstddef>
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"
#include "EditorTool.h"

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

        void HandleDrag(const Viewport& vp, bool isHovered, glm::vec3 worldSpaceHover);

        void ProcessSectorHover(const ::Viewport& vp, const ::Sector& sector, bool isHovered, const glm::vec2 screenSpaceHover, size_t s);

        void ProcessVertexHover(const ::Viewport& vp, const glm::vec2 vertexScreenSpace, const glm::vec2 screenSpaceHover, bool isHovered, ::Sector&
                                sector, const glm::vec2 endVertexScreenSpace, const glm::vec3 worldSpaceHover, size_t i, size_t s, Color& c, const glm
                                ::vec3 start_ceiling);

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
