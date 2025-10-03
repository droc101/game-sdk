//
// Created by droc101 on 10/3/25.
//

#include "SelectTool.h"

#include "../LevelEditor.h"
#include "../LevelRenderer.h"
#include "libassets/util/Color.h"
#include "libassets/util/Sector.h"
#include "VertexTool.h"

void SelectTool::RenderViewport(Viewport &vp)
{
    LevelRenderer::RenderViewport(vp);
    glm::mat4 matrix = vp.GetMatrix();

    bool isHovered = false;
    glm::vec3 worldSpaceHover{};
    glm::vec2 screenSpaceHover{};

    if (ImGui::IsWindowFocused())
    {
        isHovered = ImGui::IsWindowHovered();
        if (isHovered)
        {
            worldSpaceHover = vp.GetWorldSpaceMousePos();
            const ImVec2 localMouse = Viewport::GetLocalMousePos();
            screenSpaceHover = glm::vec2(localMouse.x, localMouse.y);
        }
    }

    hoverType = ItemType::NONE;

    for (size_t sectorIndex = 0; sectorIndex < LevelEditor::level.sectors.size(); sectorIndex++)
    {
        Sector &sector = LevelEditor::level.sectors.at(sectorIndex);
        if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ &&
            hoverType == ItemType::NONE &&
            sector.ContainsPoint({worldSpaceHover.x, worldSpaceHover.z}))
        {
            hoverIndex = sectorIndex;
            hoverType = ItemType::SECTOR;
        }

        Color c = Color(0.6, 0.6, 0.6, 1);
        if (selectionType == ItemType::SECTOR && selectionIndex == sectorIndex)
        {
            c = Color(1, 1, 1, 1);
        } else if (hoverType == ItemType::SECTOR && hoverIndex == sectorIndex)
        {
            c = Color(.8, .8, .8, 1);
        }

        for (size_t vertexIndex = 0; vertexIndex < sector.points.size(); vertexIndex++)
        {
            const std::array<float, 2> &start2 = sector.points[vertexIndex];
            const std::array<float, 2> &end2 = sector.points[(vertexIndex + 1) % sector.points.size()];
            const glm::vec3 startCeiling = glm::vec3(start2.at(0), sector.ceilingHeight, start2.at(1));
            const glm::vec3 endCeiling = glm::vec3(end2.at(0), sector.ceilingHeight, end2.at(1));
            const glm::vec3 startFloor = glm::vec3(start2.at(0), sector.floorHeight, start2.at(1));
            const glm::vec3 endFloor = glm::vec3(end2.at(0), sector.floorHeight, end2.at(1));

            if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
            {
                LevelRenderer::RenderLine(startFloor, endFloor, c, matrix, 4);
                LevelRenderer::RenderLine(startCeiling, startFloor, c, matrix, 4);
            }

            LevelRenderer::RenderLine(startCeiling, endCeiling, c, matrix, 4);
        }
    }

    if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        if ((hoverType == ItemType::NONE && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) ||
            ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
        {
            selectionType = ItemType::NONE;
        } else if (hoverType == ItemType::SECTOR && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            selectionIndex = hoverIndex;
            selectionType = ItemType::SECTOR;
        }

        if (selectionType == ItemType::SECTOR && ImGui::Shortcut(ImGuiKey_Enter, ImGuiInputFlags_RouteGlobal))
        {
            LevelEditor::toolType = LevelEditor::EditorToolType::EDIT_SECTOR;
            LevelEditor::tool = std::unique_ptr<EditorTool>(new VertexTool(selectionIndex));
        }
    }
}

void SelectTool::RenderToolWindow()
{
    if (!ImGui::CollapsingHeader("Select Tool", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    ImGui::PushItemWidth(-1);
    ImGui::Text("No Options");
}
