//
// Created by droc101 on 10/16/26.
//

#include "FaceTool.h"

#include "../MapEditor.h"
#include "../Viewport.h"
#include "../ViewportRenderer.h"
#include "imgui.h"
#include "libassets/type/Brush.h"

void FaceTool::RenderToolWindow()
{
    if (!ImGui::CollapsingHeader("Face Tool", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    if (hasSelection)
    {
        Brush &brush = MapEditor::map.brushes.at(brushIndex);
        MapEditor::BrushFaceToolWindow(brush.faces.at(faceIndex));
        brush.facesNeedRetriangulation = true;
    }
}

void FaceTool::RenderViewport(Viewport &vp)
{
    if (hasSelection)
    {
        if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
        {
            hasSelection = false;
        }
    }

    if (vp.Is3D())
    {
        const Viewport::PerspectiveHoverInfo hoverInfo = vp.GetPerspectiveHover();
        if (static_cast<ItemType>(hoverInfo.objectType) == ItemType::BRUSH)
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                hasSelection = true;
                brushIndex = hoverInfo.objectIndex;
                faceIndex = hoverInfo.objectSubIndex;
            }
        }
    }

    const ViewportRenderer::ViewportRenderSettings vps = {
        .brushFocusMode = false,
        .focusedBrushIndex = 0,
        .hoverType = ItemType::NONE,
        .hoverIndex = 0,
        .selectionType = hasSelection ? ItemType::FACE : ItemType::NONE,
        .selectionIndex = brushIndex,
        .selectionVertexIndex = faceIndex,
    };
    ViewportRenderer::RenderViewport(vp, vps);
}
