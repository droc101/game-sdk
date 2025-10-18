//
// Created by droc101 on 10/3/25.
//

#include "SelectTool.h"
#include <array>
#include <cfloat>
#include <cstddef>
#include <libassets/type/Color.h>
#include <libassets/type/Sector.h>
#include "../LevelEditor.h"
#include "../LevelRenderer.h"
#include "../Viewport.h"
#include "imgui.h"

void SelectTool::HandleDrag(const Viewport &vp, const bool isHovered, const glm::vec3 worldSpaceHover)
{
    if (!isHovered)
    {
        return;
    }
    if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        if (selectionType == ItemType::VERTEX &&
            ((hoverType == ItemType::VERTEX && hoverIndex == selectionVertexIndex) || dragging))
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                Sector &sector = LevelEditor::level.sectors.at(focusedSectorIndex);
                const glm::vec3 snapped = LevelEditor::SnapToGrid(worldSpaceHover);
                sector.points[selectionVertexIndex][0] = snapped.x;
                sector.points[selectionVertexIndex][1] = snapped.z;
                dragging = true;
            } else
            {
                dragging = false;
            }
        } else if (selectionType == ItemType::LINE &&
                   ((hoverType == ItemType::LINE && hoverIndex == selectionVertexIndex) || dragging))
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                Sector &sector = LevelEditor::level.sectors.at(focusedSectorIndex);
                const glm::vec2 worldHover2D = glm::vec2(worldSpaceHover.x, worldSpaceHover.z);
                const glm::vec2 startPos = worldHover2D - lineDragModeMouseOffset;
                const glm::vec2 endPos = startPos - lineDragModeSecondVertexOffset;
                const glm::vec3 startSnapped = LevelEditor::SnapToGrid(glm::vec3(startPos.x, 0, startPos.y));
                const glm::vec3 endSnapped = LevelEditor::SnapToGrid(glm::vec3(endPos.x, 0, endPos.y));
                sector.points[selectionVertexIndex][0] = startSnapped.x;
                sector.points[selectionVertexIndex][1] = startSnapped.z;
                sector.points[(selectionVertexIndex + 1) % sector.points.size()][0] = endSnapped.x;
                sector.points[(selectionVertexIndex + 1) % sector.points.size()][1] = endSnapped.z;
                dragging = true;
            } else
            {
                dragging = false;
            }
        }
    } else
    {
        // TODO prevent putting ceiling below floor / floor above ceiling
        if (selectionType == ItemType::CEILING && (hoverType == ItemType::CEILING || dragging))
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                Sector &sector = LevelEditor::level.sectors.at(focusedSectorIndex);
                const glm::vec3 snapped = LevelEditor::SnapToGrid(worldSpaceHover);
                sector.ceilingHeight = snapped.y;
                dragging = true;
            } else
            {
                dragging = false;
            }
        } else if (selectionType == ItemType::FLOOR && (hoverType == ItemType::FLOOR || dragging))
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                Sector &sector = LevelEditor::level.sectors.at(focusedSectorIndex);
                const glm::vec3 snapped = LevelEditor::SnapToGrid(worldSpaceHover);
                sector.floorHeight = snapped.y;
                dragging = true;
            } else
            {
                dragging = false;
            }
        }
    }
}

void SelectTool::ProcessSectorHover(const Viewport &vp,
                                    const Sector &sector,
                                    const bool isHovered,
                                    const glm::vec2 screenSpaceHover,
                                    const size_t sectorIndex)
{
    if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
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
            hoverType = ItemType::CEILING;
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            if (ImGui::BeginTooltip())
            {
                ImGui::Text("Sector %lu ceiling: %.2f units", sectorIndex + 1, sector.ceilingHeight);
                ImGui::EndTooltip();
            }
            if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                selectionType = ItemType::CEILING;
            }
        } else if (floorDistance <= 5)
        {
            hoverType = ItemType::FLOOR;
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            if (ImGui::BeginTooltip())
            {
                ImGui::Text("Sector %lu floor: %.2f units", sectorIndex + 1, sector.floorHeight);
                ImGui::EndTooltip();
            }
            if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                selectionType = ItemType::FLOOR;
            }
        }
    }
}

void SelectTool::ProcessVertexHover(const Viewport &viewport,
                                    const glm::vec2 vertexScreenSpace,
                                    const glm::vec2 screenSpaceHover,
                                    bool isHovered,
                                    Sector &sector,
                                    const glm::vec2 endVertexScreenSpace,
                                    const glm::vec3 worldSpaceHover,
                                    const size_t vertexIndex,
                                    const size_t sectorIndex,
                                    Color &vertexColor,
                                    const glm::vec3 startCeiling,
                                    Color &lineColor)
{
    if (viewport.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        if (glm::distance(vertexScreenSpace, screenSpaceHover) <= LevelEditor::HOVER_DISTANCE_PIXELS)
        {
            hoverType = ItemType::VERTEX;
            hoverIndex = vertexIndex;
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
            hoverType = ItemType::LINE;
            hoverIndex = vertexIndex;
            lineColor = Color(1, .8, .8, 1);
            const bool addPointMode = ImGui::IsKeyDown(ImGuiKey_LeftShift) ||
                                      ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
            ImGui::SetMouseCursor(addPointMode ? ImGuiMouseCursor_Hand : ImGuiMouseCursor_ResizeAll);
            if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                if (addPointMode) // TODO this freezes in a memory leak loop sometimes?!?!?!?!
                {
                    const glm::vec2 newVertexPos = LevelEditor::SnapToGrid(worldSpaceHover);
                    sector.points.insert(sector.points.begin() + static_cast<ptrdiff_t>(vertexIndex) + 1,
                                         {newVertexPos.x, newVertexPos.y});
                    sector.wallMaterials.insert(sector.wallMaterials.begin() + static_cast<ptrdiff_t>(vertexIndex) + 1,
                                                sector.wallMaterials.at(vertexIndex));
                    selectionVertexIndex = vertexIndex + 1;
                    selectionType = ItemType::VERTEX;
                    hoverType = ItemType::NONE;
                } else
                {
                    selectionVertexIndex = vertexIndex;
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

void SelectTool::RenderViewportVertexMode(Viewport &vp,
                                          glm::mat4 &matrix,
                                          bool isHovered,
                                          glm::vec3 &worldSpaceHover,
                                          glm::vec2 &screenSpaceHover)
{
    hoverType = ItemType::NONE;

    for (size_t sectorIndex = 0; sectorIndex < LevelEditor::level.sectors.size(); sectorIndex++)
    {
        Sector &sector = LevelEditor::level.sectors.at(sectorIndex);
        if (focusedSectorIndex == sectorIndex)
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
        selectionType = ItemType::SECTOR;
        selectionIndex = focusedSectorIndex;
        sectorFocusMode = false;
    }

    for (size_t sectorIndex = 0; sectorIndex < LevelEditor::level.sectors.size(); sectorIndex++)
    {
        Sector &sector = LevelEditor::level.sectors.at(sectorIndex);
        if (focusedSectorIndex == sectorIndex)
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
            Color vertexColor = Color(0.8, 0, 0, 1);
            Color lineColor = Color(1, 1, 1, 1);
            ProcessVertexHover(vp,
                               vertexScreenSpace,
                               screenSpaceHover,
                               isHovered,
                               sector,
                               endVertexScreenSpace,
                               worldSpaceHover,
                               vertexIndex,
                               sectorIndex,
                               vertexColor,
                               startCeiling,
                               lineColor);

            if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
            {
                if (selectionType == ItemType::VERTEX &&
                    selectionVertexIndex == vertexIndex &&
                    selectionIndex == sectorIndex)
                {
                    LevelRenderer::RenderBillboardPoint(startCeiling + glm::vec3(0, 0.05, 0),
                                                        12,
                                                        Color(0, 1, 1, 1),
                                                        matrix);
                }
                LevelRenderer::RenderBillboardPoint(startCeiling + glm::vec3(0, 0.1, 0), 10, vertexColor, matrix);
            } else
            {
                if (selectionType == ItemType::LINE && selectionVertexIndex == vertexIndex)
                {
                    LevelRenderer::RenderLine(startFloor, endFloor, Color(1, 0, 1, 1), matrix, 8);
                }
                LevelRenderer::RenderLine(startFloor, endFloor, lineColor, matrix, 4);
                LevelRenderer::RenderLine(startCeiling, startFloor, Color(.6, .6, .6, 1), matrix, 4);
            }

            LevelRenderer::RenderLine(startCeiling, endCeiling, lineColor, matrix, 4);
            if (selectionType == ItemType::LINE && selectionVertexIndex == vertexIndex)
            {
                LevelRenderer::RenderLine(startCeiling, endCeiling, Color(1, 0, 1, 1), matrix, 8);
            }
        }
    }

    HandleDrag(vp, isHovered, worldSpaceHover);
}


void SelectTool::RenderViewportSelectMode(const Viewport &vp,
                                          glm::mat4 &matrix,
                                          const bool isHovered,
                                          const glm::vec3 &worldSpaceHover)
{
    hoverType = ItemType::NONE;

    for (size_t sectorIndex = 0; sectorIndex < LevelEditor::level.sectors.size(); sectorIndex++)
    {
        Sector &sector = LevelEditor::level.sectors.at(sectorIndex);
        if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ &&
            hoverType == ItemType::NONE &&
            sector.ContainsPoint({worldSpaceHover.x, worldSpaceHover.z}) &&
            isHovered)
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

    if (isHovered && vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        if ((hoverType == ItemType::NONE && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) ||
            ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
        {
            selectionType = ItemType::NONE;
        } else if (hoverType == ItemType::SECTOR && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            selectionIndex = hoverIndex;
            selectionType = ItemType::SECTOR;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                focusedSectorIndex = selectionIndex;
                sectorFocusMode = true;
            }
        }

        if (selectionType == ItemType::SECTOR && (ImGui::Shortcut(ImGuiKey_Enter, ImGuiInputFlags_RouteGlobal) ||
                                                  ImGui::Shortcut(ImGuiKey_KeypadEnter, ImGuiInputFlags_RouteGlobal)))
        {
            focusedSectorIndex = selectionIndex;
            sectorFocusMode = true;
        }
    }
}


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

    if (sectorFocusMode)
    {
        RenderViewportVertexMode(vp, matrix, isHovered, worldSpaceHover, screenSpaceHover);
    } else
    {
        RenderViewportSelectMode(vp, matrix, isHovered, worldSpaceHover);
    }
}


void SelectTool::RenderToolWindow()
{
    if (!ImGui::CollapsingHeader("Select Tool", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    ImGui::PushItemWidth(-1);
    switch (selectionType)
    {
        case ItemType::NONE:
            ImGui::Text("No Selection");
            break;
        case ItemType::VERTEX:
            ImGui::InputFloat2("##vertexPosition",
                               LevelEditor::level.sectors.at(focusedSectorIndex)
                                       .points.at(selectionVertexIndex)
                                       .data());
            break;
        case ItemType::LINE:
            LevelEditor::MaterialToolWindow(LevelEditor::level.sectors.at(focusedSectorIndex)
                                                    .wallMaterials.at(selectionVertexIndex));
            break;
        case ItemType::CEILING:
            LevelEditor::MaterialToolWindow(LevelEditor::level.sectors.at(focusedSectorIndex).ceilingMaterial);
            ImGui::Separator();
            ImGui::Text("Height");
            ImGui::InputFloat("##ceilHeight", &LevelEditor::level.sectors.at(focusedSectorIndex).ceilingHeight);
            break;
        case ItemType::FLOOR:
            LevelEditor::MaterialToolWindow(LevelEditor::level.sectors.at(focusedSectorIndex).floorMaterial);
            ImGui::Separator();
            ImGui::Text("Height");
            ImGui::InputFloat("##floorHeight", &LevelEditor::level.sectors.at(focusedSectorIndex).floorHeight);
            break;
        case ItemType::SECTOR:
            ImGui::ColorEdit4("##sectorColor",
                              LevelEditor::level.sectors.at(focusedSectorIndex).lightColor.GetDataPointer());
            break;
        default:
            ImGui::Text("The current selection has no properties");
    }
}
