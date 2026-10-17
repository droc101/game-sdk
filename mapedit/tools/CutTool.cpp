//
// Created by droc101 on 9/2/26.
//

#include "CutTool.h"
#include "../ViewportRenderer.h"

void CutTool::RenderToolWindow() {}

void CutTool::RenderViewport(Viewport &vp)
{
    const ViewportRenderer::ViewportRenderSettings vps = {
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
