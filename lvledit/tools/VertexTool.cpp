//
// Created by droc101 on 9/7/25.
//

#include <array>
#include <cstddef>
#include <imgui.h>
#include "../LevelEditor.h"
#include "../LevelRenderer.h"
#include "../Viewport.h"
#include "libassets/util/Color.h"
#include "libassets/util/Sector.h"
#include "Options.h"
#include "VertexTool.h"

void VertexTool::RenderViewport(Viewport &vp)
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

    if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        if (dragType == DragType::VERTEX && isHovered)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                Sector &sector = LevelEditor::level.sectors.at(sectorIndex);
                const glm::vec3 snapped = LevelEditor::SnapToGrid(worldSpaceHover);
                sector.points[vertexIndex][0] = snapped.x;
                sector.points[vertexIndex][1] = snapped.z;
            } else
            {
                dragType = DragType::NONE;
            }
        }
    } else
    {
        // TODO prevent putting ceiling below floor / floor above ceiling
        if (dragType == DragType::CEILING && isHovered)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                Sector &sector = LevelEditor::level.sectors.at(sectorIndex);
                const glm::vec3 snapped = LevelEditor::SnapToGrid(worldSpaceHover);
                sector.ceilingHeight = snapped.y;
            } else
            {
                dragType = DragType::NONE;
            }
        } else if (dragType == DragType::FLOOR && isHovered)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                Sector &sector = LevelEditor::level.sectors.at(sectorIndex);
                const glm::vec3 snapped = LevelEditor::SnapToGrid(worldSpaceHover);
                sector.floorHeight = snapped.y;
            } else
            {
                dragType = DragType::NONE;
            }
        }
    }

    for (size_t s = 0; s < LevelEditor::level.sectors.size(); s++)
    {
        Sector &sector = LevelEditor::level.sectors.at(s);
        if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ && dragType == DragType::NONE)
        {
            const std::array<float, 4> sectorBB = sector.CalculateBBox();
            float ceilingDistance = FLT_MAX;
            float floorDistance = FLT_MAX;
            if (vp.GetType() == Viewport::ViewportType::FRONT_XY)
            {
                const glm::vec3 ceilingA = glm::vec3(sectorBB.at(0), sector.ceilingHeight, 0);
                const glm::vec3 ceilingB = glm::vec3(sectorBB.at(2), sector.ceilingHeight,0);
                ceilingDistance = LevelEditor::VecDistanceToLine2D(vp.WorldToScreenPos(ceilingA), vp.WorldToScreenPos(ceilingB), screenSpaceHover);
                const glm::vec3 floorA = glm::vec3(sectorBB.at(0), sector.floorHeight, 0);
                const glm::vec3 floorB = glm::vec3(sectorBB.at(2), sector.floorHeight,0);
                floorDistance = LevelEditor::VecDistanceToLine2D(vp.WorldToScreenPos(floorA), vp.WorldToScreenPos(floorB), screenSpaceHover);
            } else if (vp.GetType() == Viewport::ViewportType::SIDE_YZ)
            {
                const glm::vec3 ceilingA = glm::vec3(0, sector.ceilingHeight, sectorBB.at(1));
                const glm::vec3 ceilingB = glm::vec3(0, sector.ceilingHeight,sectorBB.at(3));
                ceilingDistance = LevelEditor::VecDistanceToLine2D(vp.WorldToScreenPos(ceilingA), vp.WorldToScreenPos(ceilingB), screenSpaceHover);
                const glm::vec3 floorA = glm::vec3(0, sector.floorHeight, sectorBB.at(1));
                const glm::vec3 floorB = glm::vec3(0, sector.floorHeight,sectorBB.at(3));
                floorDistance = LevelEditor::VecDistanceToLine2D(vp.WorldToScreenPos(floorA), vp.WorldToScreenPos(floorB), screenSpaceHover);
            }

            if (ceilingDistance <= 5)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                if (ImGui::BeginTooltip())
                {
                    ImGui::Text("Sector %lu ceiling: %.2f units", s, sector.ceilingHeight);
                    ImGui::EndTooltip();
                }
                if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    sectorIndex = s;
                    dragType = DragType::CEILING;
                }
            } else if (floorDistance <= 5)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                if (ImGui::BeginTooltip())
                {
                    ImGui::Text("Sector %lu floor: %.2f units", s, sector.floorHeight);
                    ImGui::EndTooltip();
                }
                if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    sectorIndex = s;
                    dragType = DragType::FLOOR;
                }
            }
        }

        for (size_t i = 0; i < sector.points.size(); i++)
        {
            const std::array<float, 2> &start2 = sector.points[i];
            const std::array<float, 2> &end2 = sector.points[(i + 1) % sector.points.size()];
            const glm::vec3 start_ceiling = glm::vec3(start2.at(0), sector.ceilingHeight, start2.at(1));
            const glm::vec3 end_ceiling = glm::vec3(end2.at(0), sector.ceilingHeight, end2.at(1));
            const glm::vec3 start_floor = glm::vec3(start2.at(0), sector.floorHeight, start2.at(1));
            const glm::vec3 end_floor = glm::vec3(end2.at(0), sector.floorHeight, end2.at(1));

            const glm::vec2 vertexScreenSpace = vp.WorldToScreenPos(start_ceiling);
            const glm::vec2 endVertexScreenSpace = vp.WorldToScreenPos(end_ceiling);
            Color c = Color(1,0,0,1);
            if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
            {
                if (dragType == DragType::NONE)
                {
                    if (distance(vertexScreenSpace, screenSpaceHover) <= 5)
                    {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                        c = Color(1,0.5,.5,1);
                        if (ImGui::BeginTooltip())
                        {
                            ImGui::Text("Sector %ld vertex %ld\n%.2f, %.2f", s, i, start_ceiling.x, start_ceiling.z);
                            ImGui::EndTooltip();
                        }
                        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            vertexIndex = i;
                            sectorIndex = s;
                            dragType = DragType::VERTEX;
                        } else if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && sector.points.size() > 3)
                        {
                            sector.points.erase(sector.points.begin() + i);
                        }
                    } else if (LevelEditor::VecDistanceToLine2D(vertexScreenSpace, endVertexScreenSpace, screenSpaceHover) <= 5 && distance(endVertexScreenSpace, screenSpaceHover) > 5)
                    {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            const glm::vec2 newVertexPos = LevelEditor::SnapToGrid(worldSpaceHover);
                            sector.points.insert(sector.points.begin() + i + 1, {newVertexPos.x, newVertexPos.y});
                            sector.wallMaterials.insert(sector.wallMaterials.begin() + i + 1, sector.wallMaterials.at(i));
                            vertexIndex = i+1;
                            sectorIndex = s;
                            dragType = DragType::VERTEX;
                        }
                    }
                }
            }


            if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
            {
                LevelRenderer::RenderBillboardPoint(start_ceiling + glm::vec3(0, 0.1, 0), 10, c, matrix);
            } else
            {
                LevelRenderer::RenderLine(start_floor, end_floor, Color(1, 1, 1, 1), matrix, 4);
                LevelRenderer::RenderLine(start_ceiling, start_floor, Color(.6, .6, .6, 1), matrix, 4);
            }

            LevelRenderer::RenderLine(start_ceiling, end_ceiling, Color(1, 1, 1, 1), matrix, 4);
        }
    }
}

void VertexTool::RenderToolWindow()
{

}
