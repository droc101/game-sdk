//
// Created by droc101 on 9/3/26.
//

#include "VertexTool.h"
#include "../Viewport.h"
#include "../ViewportRenderer.h"

void VertexTool::RenderToolWindow() {}

void VertexTool::RenderViewport(Viewport &vp)
{
    constexpr ViewportRenderer::ViewportRenderSettings vps = {
        .brushFocusMode = false,
        .focusedBrushIndex = 0,
        .hoverType = ItemType::NONE,
        .hoverIndex = 0,
        .selectionType = ItemType::NONE,
        .selectionIndex = 0,
        .selectionSubIndex = 0,
    };
    ViewportRenderer::RenderViewport(vp, vps);
}
