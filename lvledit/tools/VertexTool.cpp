//
// Created by droc101 on 9/7/25.
//

#include "VertexTool.h"
#include <array>
#include <cfloat>
#include <cstddef>
#include <imgui.h>
#include "../LevelEditor.h"
#include "../LevelRenderer.h"
#include "../Viewport.h"
#include "libassets/util/Color.h"
#include "libassets/util/Sector.h"

VertexTool::VertexTool(const size_t sectorIndex)
{
    sectorLockIndex = sectorIndex;
    sectorLock = true;
}

void VertexTool::HandleDrag(const Viewport &vp, const bool isHovered, const glm::vec3 worldSpaceHover)
{
    if (!isHovered)
    {
        return;
    }
    if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        if (selectionType == ItemType::VERTEX)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                Sector &sector = LevelEditor::level.sectors.at(selectionSectorIndex);
                const glm::vec3 snapped = LevelEditor::SnapToGrid(worldSpaceHover);
                sector.points[selectionVertexIndex][0] = snapped.x;
                sector.points[selectionVertexIndex][1] = snapped.z;
            }
        } else if (selectionType == ItemType::LINE)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                Sector &sector = LevelEditor::level.sectors.at(selectionSectorIndex);
                const glm::vec2 worldHover2D = glm::vec2(worldSpaceHover.x, worldSpaceHover.z);
                const glm::vec2 startPos = worldHover2D - lineDragModeMouseOffset;
                const glm::vec2 endPos = startPos - lineDragModeSecondVertexOffset;
                const glm::vec3 startSnapped = LevelEditor::SnapToGrid(glm::vec3(startPos.x, 0, startPos.y));
                const glm::vec3 endSnapped = LevelEditor::SnapToGrid(glm::vec3(endPos.x, 0, endPos.y));
                sector.points[selectionVertexIndex][0] = startSnapped.x;
                sector.points[selectionVertexIndex][1] = startSnapped.z;
                sector.points[(selectionVertexIndex + 1) % sector.points.size()][0] = endSnapped.x;
                sector.points[(selectionVertexIndex + 1) % sector.points.size()][1] = endSnapped.z;
            }
        }
    } else
    {
        // TODO prevent putting ceiling below floor / floor above ceiling
        if (selectionType == ItemType::CEILING)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                Sector &sector = LevelEditor::level.sectors.at(selectionSectorIndex);
                const glm::vec3 snapped = LevelEditor::SnapToGrid(worldSpaceHover);
                sector.ceilingHeight = snapped.y;
            }
        } else if (selectionType == ItemType::FLOOR)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                Sector &sector = LevelEditor::level.sectors.at(selectionSectorIndex);
                const glm::vec3 snapped = LevelEditor::SnapToGrid(worldSpaceHover);
                sector.floorHeight = snapped.y;
            }
        }
    }
}

void VertexTool::ProcessSectorHover(const Viewport &vp,
                                    const Sector &sector,
                                    const bool isHovered,
                                    const glm::vec2 screenSpaceHover,
                                    const size_t sectorIndex)
{
    if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ) // && selectionType == ItemType::NONE)
    {
        const std::array<float, 4> sectorBB = LevelEditor::CalculateBBox(sector.points);
        float ceilingDistance = FLT_MAX;
        float floorDistance = FLT_MAX;
        if (vp.GetType() == Viewport::ViewportType::FRONT_XY)
        {
            const glm::vec3 ceilingA = glm::vec3(sectorBB.at(0), sector.ceilingHeight, 0);
            const glm::vec3 ceilingB = glm::vec3(sectorBB.at(2), sector.ceilingHeight, 0);
            ceilingDistance = LevelEditor::VecDistanceToLine2D(vp.WorldToScreenPos(ceilingA),
                                                               vp.WorldToScreenPos(ceilingB),
                                                               screenSpaceHover);
            const glm::vec3 floorA = glm::vec3(sectorBB.at(0), sector.floorHeight, 0);
            const glm::vec3 floorB = glm::vec3(sectorBB.at(2), sector.floorHeight, 0);
            floorDistance = LevelEditor::VecDistanceToLine2D(vp.WorldToScreenPos(floorA),
                                                             vp.WorldToScreenPos(floorB),
                                                             screenSpaceHover);
        } else if (vp.GetType() == Viewport::ViewportType::SIDE_YZ)
        {
            const glm::vec3 ceilingA = glm::vec3(0, sector.ceilingHeight, sectorBB.at(1));
            const glm::vec3 ceilingB = glm::vec3(0, sector.ceilingHeight, sectorBB.at(3));
            ceilingDistance = LevelEditor::VecDistanceToLine2D(vp.WorldToScreenPos(ceilingA),
                                                               vp.WorldToScreenPos(ceilingB),
                                                               screenSpaceHover);
            const glm::vec3 floorA = glm::vec3(0, sector.floorHeight, sectorBB.at(1));
            const glm::vec3 floorB = glm::vec3(0, sector.floorHeight, sectorBB.at(3));
            floorDistance = LevelEditor::VecDistanceToLine2D(vp.WorldToScreenPos(floorA),
                                                             vp.WorldToScreenPos(floorB),
                                                             screenSpaceHover);
        }

        if (ceilingDistance <= 5)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            if (ImGui::BeginTooltip())
            {
                ImGui::Text("Sector %lu ceiling: %.2f units", sectorIndex + 1, sector.ceilingHeight);
                ImGui::EndTooltip();
            }
            if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                selectionSectorIndex = sectorIndex;
                selectionType = ItemType::CEILING;
            }
        } else if (floorDistance <= 5)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            if (ImGui::BeginTooltip())
            {
                ImGui::Text("Sector %lu floor: %.2f units", sectorIndex + 1, sector.floorHeight);
                ImGui::EndTooltip();
            }
            if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                selectionSectorIndex = sectorIndex;
                selectionType = ItemType::FLOOR;
            }
        }
    }
}

void VertexTool::ProcessVertexHover(const Viewport &viewport,
                                    const glm::vec2 vertexScreenSpace,
                                    const glm::vec2 screenSpaceHover,
                                    bool isHovered,
                                    Sector &sector,
                                    const glm::vec2 endVertexScreenSpace,
                                    const glm::vec3 worldSpaceHover,
                                    const size_t vertexIndex,
                                    const size_t sectorIndex,
                                    Color &vertexColor,
                                    const glm::vec3 startCeiling)
{
    if (viewport.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        if (glm::distance(vertexScreenSpace, screenSpaceHover) <= LevelEditor::HOVER_DISTANCE_PIXELS)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            vertexColor = Color(1, 0.5, .5, 1);
            if (ImGui::BeginTooltip())
            {
                ImGui::Text("Sector %ld vertex %ld\n%.2f, %.2f",
                            sectorIndex + 1,
                            vertexIndex + 1,
                            startCeiling.x,
                            startCeiling.z);
                ImGui::EndTooltip();
            }
            if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                selectionVertexIndex = vertexIndex;
                selectionSectorIndex = sectorIndex;
                selectionType = ItemType::VERTEX;
            } else if (isHovered &&
                       (ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
                        ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_RouteGlobal)) &&
                       sector.points.size() > 3)
            {
                sector.points.erase(sector.points.begin() + static_cast<ptrdiff_t>(vertexIndex));
            }
            return;
        }
        const float distanceToLine = LevelEditor::VecDistanceToLine2D(vertexScreenSpace,
                                                                      endVertexScreenSpace,
                                                                      screenSpaceHover);
        if (distanceToLine <= LevelEditor::HOVER_DISTANCE_PIXELS &&
            glm::distance(endVertexScreenSpace, screenSpaceHover) > LevelEditor::HOVER_DISTANCE_PIXELS)
        {
            const bool addPointMode = ImGui::IsKeyDown(ImGuiKey_LeftShift) ||
                                      ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
            ImGui::SetMouseCursor(addPointMode ? ImGuiMouseCursor_Hand : ImGuiMouseCursor_ResizeAll);
            if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                if (addPointMode)
                {
                    const glm::vec2 newVertexPos = LevelEditor::SnapToGrid(worldSpaceHover);
                    sector.points.insert(sector.points.begin() + static_cast<ptrdiff_t>(vertexIndex) + 1,
                                         {newVertexPos.x, newVertexPos.y});
                    sector.wallMaterials.insert(sector.wallMaterials.begin() + static_cast<ptrdiff_t>(vertexIndex) + 1,
                                                sector.wallMaterials.at(vertexIndex));
                    selectionVertexIndex = vertexIndex + 1;
                    selectionSectorIndex = sectorIndex;
                    selectionType = ItemType::VERTEX;
                } else
                {
                    selectionVertexIndex = vertexIndex;
                    selectionSectorIndex = sectorIndex;
                    const glm::vec2 startPoint{
                        sector.points.at(vertexIndex).at(0),
                        sector.points.at(vertexIndex).at(1),
                    };
                    const glm::vec2 endPoint{
                        sector.points.at((vertexIndex + 1) % sector.points.size()).at(0),
                        sector.points.at((vertexIndex + 1) % sector.points.size()).at(1),
                    };
                    const glm::vec2 worldHover2D{worldSpaceHover.x, worldSpaceHover.z};
                    lineDragModeSecondVertexOffset = startPoint - endPoint;
                    lineDragModeMouseOffset = worldHover2D - startPoint;
                    selectionType = ItemType::LINE;
                }
            }
        }
    }
}


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

    if (sectorLock)
    {
        for (size_t sectorIndex = 0; sectorIndex < LevelEditor::level.sectors.size(); sectorIndex++)
        {
            Sector &sector = LevelEditor::level.sectors.at(sectorIndex);
            if (sectorLockIndex == sectorIndex)
            {
                continue;
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
                    LevelRenderer::RenderLine(startFloor, endFloor, Color(0.5, 0.5, 0.5, 1), matrix, 4);
                    LevelRenderer::RenderLine(startCeiling, startFloor, Color(.3, .3, .3, 1), matrix, 4);
                }

                LevelRenderer::RenderLine(startCeiling, endCeiling, Color(0.5, 0.5, 0.5, 1), matrix, 4);
            }
        }

        if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
        {
            LevelEditor::toolType = LevelEditor::EditorToolType::SELECT;
            LevelEditor::tool = std::unique_ptr<EditorTool>(new SelectTool());
        }
    }

    for (size_t sectorIndex = 0; sectorIndex < LevelEditor::level.sectors.size(); sectorIndex++)
    {
        Sector &sector = LevelEditor::level.sectors.at(sectorIndex);
        if (!sectorLock || sectorLockIndex == sectorIndex)
        {
            ProcessSectorHover(vp, sector, isHovered, screenSpaceHover, sectorIndex);
        } else
        {
            continue;
        }

        for (size_t vertexIndex = 0; vertexIndex < sector.points.size(); vertexIndex++)
        {
            const std::array<float, 2> &start2 = sector.points[vertexIndex];
            const std::array<float, 2> &end2 = sector.points[(vertexIndex + 1) % sector.points.size()];
            const glm::vec3 startCeiling = glm::vec3(start2.at(0), sector.ceilingHeight, start2.at(1));
            const glm::vec3 endCeiling = glm::vec3(end2.at(0), sector.ceilingHeight, end2.at(1));
            const glm::vec3 startFloor = glm::vec3(start2.at(0), sector.floorHeight, start2.at(1));
            const glm::vec3 endFloor = glm::vec3(end2.at(0), sector.floorHeight, end2.at(1));

            const glm::vec2 vertexScreenSpace = vp.WorldToScreenPos(startCeiling);
            const glm::vec2 endVertexScreenSpace = vp.WorldToScreenPos(endCeiling);
            Color c = Color(1, 0, 0, 1);
            ProcessVertexHover(vp,
                               vertexScreenSpace,
                               screenSpaceHover,
                               isHovered,
                               sector,
                               endVertexScreenSpace,
                               worldSpaceHover,
                               vertexIndex,
                               sectorIndex,
                               c,
                               startCeiling);

            if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
            {
                LevelRenderer::RenderBillboardPoint(startCeiling + glm::vec3(0, 0.1, 0), 10, c, matrix);
            } else
            {
                LevelRenderer::RenderLine(startFloor, endFloor, Color(1, 1, 1, 1), matrix, 4);
                LevelRenderer::RenderLine(startCeiling, startFloor, Color(.6, .6, .6, 1), matrix, 4);
            }

            LevelRenderer::RenderLine(startCeiling, endCeiling, Color(1, 1, 1, 1), matrix, 4);
        }
    }

    HandleDrag(vp, isHovered, worldSpaceHover);
}

void VertexTool::RenderToolWindow()
{
    if (!ImGui::CollapsingHeader("Sector Editor", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    ImGui::PushItemWidth(-1);
    WallMaterial *mat = nullptr;
    if (selectionType == ItemType::CEILING)
    {
        mat = &LevelEditor::level.sectors.at(selectionSectorIndex).ceilingMaterial;
    } else if (selectionType == ItemType::FLOOR)
    {
        mat = &LevelEditor::level.sectors.at(selectionSectorIndex).floorMaterial;
    } else if (selectionType == ItemType::LINE)
    {
        mat = &LevelEditor::level.sectors.at(selectionSectorIndex).wallMaterials.at(selectionVertexIndex);
    }
    if (mat != nullptr)
    {
        ImGui::Text("Current Texture: %s", mat->texture.c_str());
        if (ImGui::Button("Apply Selected Texture"))
        {
            mat->texture = LevelEditor::texture;
        }
        ImGui::Text("UV Offset");
        ImGui::InputFloat2("##UV Offset", mat->uvOffset.data());
        ImGui::Text("UV Scale");
        ImGui::InputFloat2("##UV Scale", mat->uvScale.data());
    } else
    {
        ImGui::Text("No Options");
    }
}
