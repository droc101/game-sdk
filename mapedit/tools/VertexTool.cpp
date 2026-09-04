//
// Created by droc101 on 9/3/26.
//

#include "VertexTool.h"
#include "../ViewportRenderer.h"

void VertexTool::RenderToolWindow() {}

void VertexTool::RenderViewport(Viewport &vp)
{
    const ViewportRenderer::ViewportRenderSettings vps = {
        .brushFocusMode = false,
        .focusedBrushIndex = 0,
        .hoverType = ItemType::NONE,
        .hoverIndex = 0,
        .selectionType = ItemType::NONE,
        .selectionIndex = 0,
        .selectionVertexIndex = 0,
    };
    ViewportRenderer::RenderViewport(vp, vps);
}
