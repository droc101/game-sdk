//
// Created by droc101 on 10/16/26.
//

#include "FaceTool.h"
#include <cstddef>
#include <libassets/type/Brush.h>
#include <memory>
#include "../MapEditor.h"
#include "../Viewport.h"
#include "../ViewportRenderer.h"
#include "imgui.h"
#include "SelectTool.h"

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
    bool isHovered = false;

    if (ImGui::IsWindowFocused())
    {
        isHovered = ImGui::IsWindowHovered();
    }

    bool hover = false;
    size_t hoverBrush = 0;
    size_t hoverFace = 0;
    if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
    {
        if (hasSelection)
        {
            hasSelection = false;
        } else
        {
            MapEditor::toolType = MapEditor::EditorToolType::SELECT;
            MapEditor::tool = std::make_unique<SelectTool>();
        }
    }

    if (vp.Is3D() && isHovered)
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
            hover = true;
            hoverBrush = hoverInfo.objectIndex;
            hoverFace = hoverInfo.objectSubIndex;
        } else if (hoverInfo.objectType == 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            hasSelection = false;
        }
    }

    const ViewportRenderer::ViewportRenderSettings vps = {
        .hoverType = hover ? ItemType::FACE : ItemType::NONE,
        .hoverIndex = hoverBrush,
        .hoverSubIndex = hoverFace,
        .selectionType = hasSelection ? ItemType::FACE : ItemType::NONE,
        .selectionIndex = brushIndex,
        .selectionSubIndex = faceIndex,
    };
    ViewportRenderer::RenderViewport(vp, vps);
}
