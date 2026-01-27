//
// Created by droc101 on 10/3/25.
//

#include "SelectTool.h"
#include <array>
#include <cfloat>
#include <cstddef>
#include <libassets/type/Color.h>
#include <libassets/type/Sector.h>
#include <string>
#include <variant>
#include "../EditActorWindow.h"
#include "../MapEditor.h"
#include "../MapRenderer.h"
#include "../Viewport.h"
#include "imgui.h"
#include "imgui_internal.h"

void SelectTool::HandleDrag(const Viewport &vp, const bool isHovered, const glm::vec3 worldSpaceHover)
{
    if (!isHovered)
    {
        return;
    }
    if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        if (selectionType == ItemType::ACTOR &&
            ((hoverType == ItemType::ACTOR && hoverIndex == selectionIndex) || dragging))
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                Actor &actor = MapEditor::map.actors.at(selectionIndex);
                const glm::vec3 snapped = MapEditor::SnapToGrid(worldSpaceHover);
                actor.position[0] = snapped.x;
                actor.position[2] = snapped.z;
                dragging = true;
            } else
            {
                dragging = false;
            }
        } else if (selectionType == ItemType::VERTEX &&
                   ((hoverType == ItemType::VERTEX && hoverIndex == selectionVertexIndex) || dragging))
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                Sector &sector = MapEditor::map.sectors.at(focusedSectorIndex);
                const glm::vec3 snapped = MapEditor::SnapToGrid(worldSpaceHover);
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
                Sector &sector = MapEditor::map.sectors.at(focusedSectorIndex);
                const glm::vec2 worldHover2D = glm::vec2(worldSpaceHover.x, worldSpaceHover.z);
                const glm::vec2 startPos = worldHover2D - lineDragModeMouseOffset;
                const glm::vec2 endPos = startPos - lineDragModeSecondVertexOffset;
                const glm::vec3 startSnapped = MapEditor::SnapToGrid(glm::vec3(startPos.x, 0, startPos.y));
                const glm::vec3 endSnapped = MapEditor::SnapToGrid(glm::vec3(endPos.x, 0, endPos.y));
                sector.points[selectionVertexIndex][0] = startSnapped.x;
                sector.points[selectionVertexIndex][1] = startSnapped.z;
                sector.points[(selectionVertexIndex + 1) % sector.points.size()][0] = endSnapped.x;
                sector.points[(selectionVertexIndex + 1) % sector.points.size()][1] = endSnapped.z;
                dragging = true;
            } else
            {
                dragging = false;
            }
        } else if (selectionType == ItemType::SECTOR &&
                   ((hoverType == ItemType::SECTOR && hoverIndex == selectionIndex) || dragging))
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                Sector &sector = MapEditor::map.sectors.at(selectionIndex);
                if (sectorDragVertexOffsets.empty())
                {
                    const glm::vec2 worldHover2D{worldSpaceHover.x, worldSpaceHover.z};
                    const glm::vec2 firstVertex = {
                        sector.points.at(0).at(0),
                        sector.points.at(0).at(1),
                    };
                    sectorDragMouseOffset = {
                        worldHover2D.x - firstVertex.x,
                        worldHover2D.y - firstVertex.y,
                    };

                    sectorDragVertexOffsets.clear();

                    for (std::array<float, 2> &point: MapEditor::map.sectors.at(selectionIndex).points)
                    {
                        const glm::vec2 glmPoint = {point.at(0), point.at(1)};
                        sectorDragVertexOffsets.push_back(firstVertex - glmPoint);
                    }
                }
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                const glm::vec2 worldHover2D = glm::vec2(worldSpaceHover.x, worldSpaceHover.z);
                const glm::vec2 startPos = worldHover2D - sectorDragMouseOffset;
                for (size_t i = 0; i < sector.points.size(); i++)
                {
                    glm::vec2 glmPoint = startPos - sectorDragVertexOffsets.at(i);
                    const glm::vec3 snapped = MapEditor::SnapToGrid(glm::vec3(glmPoint.x, 0, glmPoint.y));
                    sector.points.at(i) = {snapped.x, snapped.z};
                }

                dragging = true;
            } else
            {
                sectorDragVertexOffsets.clear();
                dragging = false;
            }
        }
    } else
    {
        // TODO prevent putting ceiling below floor / floor above ceiling
        if (selectionType == ItemType::ACTOR &&
            ((hoverType == ItemType::ACTOR && hoverIndex == selectionIndex) || dragging))
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                Actor &actor = MapEditor::map.actors.at(selectionIndex);
                const glm::vec3 snapped = MapEditor::SnapToGrid(worldSpaceHover);
                if (vp.GetType() == Viewport::ViewportType::SIDE_YZ)
                {
                    actor.position[1] = snapped.y;
                    actor.position[2] = snapped.z;
                } else
                {
                    actor.position[0] = snapped.x;
                    actor.position[1] = snapped.y;
                }
                dragging = true;
            } else
            {
                dragging = false;
            }
        } else if (selectionType == ItemType::CEILING && (hoverType == ItemType::CEILING || dragging))
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                Sector &sector = MapEditor::map.sectors.at(focusedSectorIndex);
                const glm::vec3 snapped = MapEditor::SnapToGrid(worldSpaceHover);
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
                Sector &sector = MapEditor::map.sectors.at(focusedSectorIndex);
                const glm::vec3 snapped = MapEditor::SnapToGrid(worldSpaceHover);
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
        const std::array<float, 4> sectorBB = MapEditor::CalculateBBox(sector.points);
        float ceilingDistance = FLT_MAX;
        float floorDistance = FLT_MAX;
        if (vp.GetType() == Viewport::ViewportType::FRONT_XY)
        {
            const glm::vec3 ceilingA = glm::vec3(sectorBB.at(0), sector.ceilingHeight, 0);
            const glm::vec3 ceilingB = glm::vec3(sectorBB.at(2), sector.ceilingHeight, 0);
            ceilingDistance = MapEditor::VecDistanceToLine2D(vp.WorldToScreenPos(ceilingA),
                                                             vp.WorldToScreenPos(ceilingB),
                                                             screenSpaceHover);
            const glm::vec3 floorA = glm::vec3(sectorBB.at(0), sector.floorHeight, 0);
            const glm::vec3 floorB = glm::vec3(sectorBB.at(2), sector.floorHeight, 0);
            floorDistance = MapEditor::VecDistanceToLine2D(vp.WorldToScreenPos(floorA),
                                                           vp.WorldToScreenPos(floorB),
                                                           screenSpaceHover);
        } else if (vp.GetType() == Viewport::ViewportType::SIDE_YZ)
        {
            const glm::vec3 ceilingA = glm::vec3(0, sector.ceilingHeight, sectorBB.at(1));
            const glm::vec3 ceilingB = glm::vec3(0, sector.ceilingHeight, sectorBB.at(3));
            ceilingDistance = MapEditor::VecDistanceToLine2D(vp.WorldToScreenPos(ceilingA),
                                                             vp.WorldToScreenPos(ceilingB),
                                                             screenSpaceHover);
            const glm::vec3 floorA = glm::vec3(0, sector.floorHeight, sectorBB.at(1));
            const glm::vec3 floorB = glm::vec3(0, sector.floorHeight, sectorBB.at(3));
            floorDistance = MapEditor::VecDistanceToLine2D(vp.WorldToScreenPos(floorA),
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
                                    Color &lineColor,
                                    bool &haveAddedNewVertex)
{
    if (viewport.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        if (glm::distance(vertexScreenSpace, screenSpaceHover) <= MapEditor::HOVER_DISTANCE_PIXELS)
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
        const float distanceToLine = MapEditor::VecDistanceToLine2D(vertexScreenSpace,
                                                                    endVertexScreenSpace,
                                                                    screenSpaceHover);
        if (distanceToLine <= MapEditor::HOVER_DISTANCE_PIXELS &&
            glm::distance(endVertexScreenSpace, screenSpaceHover) > MapEditor::HOVER_DISTANCE_PIXELS)
        {
            hoverType = ItemType::LINE;
            hoverIndex = vertexIndex;
            lineColor = Color(1, .8, .8, 1);
            const bool addPointMode = !haveAddedNewVertex && (ImGui::IsKeyDown(ImGuiKey_LeftShift) ||
                                                              ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left));
            ImGui::SetMouseCursor(addPointMode ? ImGuiMouseCursor_Hand : ImGuiMouseCursor_ResizeAll);
            if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                if (addPointMode)
                {
                    const glm::vec3 newVertexPos = MapEditor::SnapToGrid(worldSpaceHover);
                    sector.points.insert(sector.points.begin() + static_cast<ptrdiff_t>(vertexIndex) + 1,
                                         {newVertexPos.x, newVertexPos.z});
                    sector.wallMaterials.insert(sector.wallMaterials.begin() + static_cast<ptrdiff_t>(vertexIndex) + 1,
                                                sector.wallMaterials.at(vertexIndex));
                    selectionVertexIndex = vertexIndex + 1;
                    selectionType = ItemType::VERTEX;
                    hoverType = ItemType::NONE;
                    haveAddedNewVertex = true;
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

    for (size_t sectorIndex = 0; sectorIndex < MapEditor::map.sectors.size(); sectorIndex++)
    {
        Sector &sector = MapEditor::map.sectors.at(sectorIndex);
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
                MapRenderer::RenderLine(startFloor, endFloor, Color(0.5, 0.5, 0.5, 1), matrix, 4);
                MapRenderer::RenderLine(startCeiling, startFloor, Color(.3, .3, .3, 1), matrix, 4);
            }

            MapRenderer::RenderLine(startCeiling, endCeiling, Color(0.5, 0.5, 0.5, 1), matrix, 4);
        }
    }

    if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
    {
        selectionType = ItemType::SECTOR;
        selectionIndex = focusedSectorIndex;
        sectorFocusMode = false;
    }

    bool haveAddedNewVertex = false;

    for (size_t sectorIndex = 0; sectorIndex < MapEditor::map.sectors.size(); sectorIndex++)
    {
        Sector &sector = MapEditor::map.sectors.at(sectorIndex);
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
                               lineColor,
                               haveAddedNewVertex);

            if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
            {
                if (selectionType == ItemType::VERTEX &&
                    selectionVertexIndex == vertexIndex &&
                    selectionIndex == sectorIndex)
                {
                    MapRenderer::RenderBillboardPoint(startCeiling + glm::vec3(0, 0.05, 0),
                                                      12,
                                                      Color(0, 1, 1, 1),
                                                      matrix);
                }
                MapRenderer::RenderBillboardPoint(startCeiling + glm::vec3(0, 0.1, 0), 10, vertexColor, matrix);
            } else
            {
                if (selectionType == ItemType::LINE && selectionVertexIndex == vertexIndex)
                {
                    MapRenderer::RenderLine(startFloor, endFloor, Color(1, 0, 1, 1), matrix, 8);
                }
                MapRenderer::RenderLine(startFloor, endFloor, lineColor, matrix, 4);
                MapRenderer::RenderLine(startCeiling, startFloor, Color(.6, .6, .6, 1), matrix, 4);
            }

            MapRenderer::RenderLine(startCeiling, endCeiling, lineColor, matrix, 4);
            if (selectionType == ItemType::LINE && selectionVertexIndex == vertexIndex)
            {
                MapRenderer::RenderLine(startCeiling, endCeiling, Color(1, 0, 1, 1), matrix, 8);
            }
        }
    }

    for (const Actor &a: MapEditor::map.actors)
    {
        MapRenderer::RenderActor(a, matrix);
    }

    HandleDrag(vp, isHovered, worldSpaceHover);

    if (ImGui::Shortcut(ImGuiKey_Delete))
    {
        if (selectionType == ItemType::VERTEX || selectionType == ItemType::LINE)
        {
            Sector &s = MapEditor::map.sectors.at(selectionIndex);
            if (s.points.size() > 3)
            {
                s.points.erase(s.points.begin() + selectionVertexIndex);
                selectionType = ItemType::NONE;
            }
        }
    }
}


void SelectTool::RenderViewportSelectMode(const Viewport &vp,
                                          glm::mat4 &matrix,
                                          const bool isHovered,
                                          const glm::vec3 &worldSpaceHover)
{
    hoverType = ItemType::NONE;

    for (size_t actorIndex = 0; actorIndex < MapEditor::map.actors.size(); actorIndex++)
    {
        Actor &a = MapEditor::map.actors.at(actorIndex);
        const glm::vec3 pos = glm::vec3(a.position.at(0), a.position.at(1), a.position.at(2));
        const glm::vec2 posScreenSpace = vp.WorldToScreenPos(pos);
        const ImVec2 hoverScreenSpaceIV = Viewport::GetLocalMousePos();
        const glm::vec2 hoverScreenSpace = glm::vec2(hoverScreenSpaceIV.x, hoverScreenSpaceIV.y);

        if (hoverType == ItemType::NONE &&
            distance(posScreenSpace, hoverScreenSpace) <= MapEditor::HOVER_DISTANCE_PIXELS &&
            isHovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            hoverIndex = actorIndex;
            hoverType = ItemType::ACTOR;
            if (ImGui::BeginTooltip())
            {
                if (a.params.contains("name") && !a.params.at("name").Get<std::string>("").empty())
                {
                    ImGui::Text("%s: %s", a.params.at("name").Get<std::string>("").c_str(), a.className.c_str());
                } else
                {
                    ImGui::Text("%s", a.className.c_str());
                }
                ImGui::Text("%.2f, %.2f, %.2f", a.position.at(0), a.position.at(1), a.position.at(2));
                ImGui::EndTooltip();
            }
        }

        MapRenderer::RenderActor(a, matrix);
    }

    for (size_t sectorIndex = 0; sectorIndex < MapEditor::map.sectors.size(); sectorIndex++)
    {
        Sector &sector = MapEditor::map.sectors.at(sectorIndex);
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
                MapRenderer::RenderLine(startFloor, endFloor, c, matrix, 4);
                MapRenderer::RenderLine(startCeiling, startFloor, c, matrix, 4);
            }

            MapRenderer::RenderLine(startCeiling, endCeiling, c, matrix, 4);
        }
    }

    if (isHovered)
    {
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
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    focusedSectorIndex = selectionIndex;
                    sectorFocusMode = true;
                }
            }
        }
        if (hoverType == ItemType::ACTOR && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            selectionIndex = hoverIndex;
            selectionType = ItemType::ACTOR;
        }

        if (selectionType == ItemType::SECTOR && (ImGui::Shortcut(ImGuiKey_Enter, ImGuiInputFlags_RouteGlobal) ||
                                                  ImGui::Shortcut(ImGuiKey_KeypadEnter, ImGuiInputFlags_RouteGlobal)))
        {
            focusedSectorIndex = selectionIndex;
            sectorFocusMode = true;
        } else if (selectionType == ItemType::ACTOR)
        {
            if (ImGui::Shortcut(ImGuiMod_Alt | ImGuiKey_Enter))
            {
                EditActorWindow::selectedParam = 0;
                EditActorWindow::visible = true;
            }
        }
    }

    HandleDrag(vp, isHovered, worldSpaceHover);

    if (ImGui::Shortcut(ImGuiKey_Delete))
    {
        if (selectionType == ItemType::ACTOR)
        {
            MapEditor::map.actors.erase(MapEditor::map.actors.begin() + selectionIndex);
            selectionType = ItemType::NONE;
        } else if (selectionType == ItemType::SECTOR)
        {
            MapEditor::map.sectors.erase(MapEditor::map.sectors.begin() + selectionIndex);
            selectionType = ItemType::NONE;
        }
    }
}


void SelectTool::RenderViewport(Viewport &vp)
{
    MapRenderer::RenderViewport(vp);
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

    if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ && selectionType == ItemType::ACTOR)
    {
        EditActorWindow::Render(MapEditor::map.actors.at(selectionIndex));
    }
}


void SelectTool::RenderToolWindow()
{
    if (!ImGui::CollapsingHeader("Select Tool", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    ImGui::PushItemWidth(-1);
    std::array<float, 4> col{};
    size_t sectIndex = 0;
    switch (selectionType)
    {
        case ItemType::NONE:
            ImGui::Text("No Selection");
            break;
        case ItemType::VERTEX:
            ImGui::InputFloat2("##vertexPosition",
                               MapEditor::map.sectors.at(focusedSectorIndex).points.at(selectionVertexIndex).data());
            break;
        case ItemType::LINE:
            MapEditor::MaterialToolWindow(MapEditor::map.sectors.at(focusedSectorIndex)
                                                  .wallMaterials.at(selectionVertexIndex));
            break;
        case ItemType::CEILING:
            MapEditor::MaterialToolWindow(MapEditor::map.sectors.at(focusedSectorIndex).ceilingMaterial);
            ImGui::Separator();
            ImGui::Text("Height");
            ImGui::InputFloat("##ceilHeight", &MapEditor::map.sectors.at(focusedSectorIndex).ceilingHeight);
            break;
        case ItemType::FLOOR:
            MapEditor::MaterialToolWindow(MapEditor::map.sectors.at(focusedSectorIndex).floorMaterial);
            ImGui::Separator();
            ImGui::Text("Height");
            ImGui::InputFloat("##floorHeight", &MapEditor::map.sectors.at(focusedSectorIndex).floorHeight);
            break;
        case ItemType::SECTOR:
            if (sectorFocusMode)
            {
                sectIndex = focusedSectorIndex;
            } else
            {
                sectIndex = selectionIndex;
            }
            col = MapEditor::map.sectors.at(sectIndex).lightColor.CopyData();
            if (ImGui::ColorEdit4("##sectorColor", col.data()))
            {
                MapEditor::map.sectors.at(sectIndex).lightColor = Color(col[0], col[1], col[2], col[3]);
            }
            break;
        case ItemType::ACTOR:
            ImGui::Text("Position");
            ImGui::InputFloat3("##position", MapEditor::map.actors.at(selectionIndex).position.data());
            ImGui::Text("Rotation");
            ImGui::InputFloat3("##rotation", MapEditor::map.actors.at(selectionIndex).rotation.data());
            ImGui::Separator();
            if (ImGui::Button("Actor Properties"))
            {
                EditActorWindow::selectedParam = 0;
                EditActorWindow::visible = true;
            }
            break;
        default:
            ImGui::Text("The current selection has no properties");
    }
}

bool SelectTool::IsCopyableSelected() const
{
    return selectionType == ItemType::ACTOR || selectionType == ItemType::SECTOR;
}

void SelectTool::Copy() const
{
    if (selectionType == ItemType::ACTOR)
    {
        MapEditor::clipboard = MapEditor::map.actors.at(selectionIndex);
    } else if (selectionType == ItemType::SECTOR)
    {
        MapEditor::clipboard = MapEditor::map.sectors.at(selectionIndex);
    }
}

void SelectTool::Cut()
{
    Copy();
    if (selectionType == ItemType::ACTOR)
    {
        MapEditor::map.actors.erase(MapEditor::map.actors.begin() + selectionIndex);
    } else if (selectionType == ItemType::SECTOR)
    {
        MapEditor::map.sectors.erase(MapEditor::map.sectors.begin() + selectionIndex);
    }
    selectionType = ItemType::NONE;
}

void SelectTool::Paste()
{
    // TODO somehow get mouse position (which VP is focused?) and paste at cursor
    if (!MapEditor::clipboard.has_value())
    {
        return;
    }
    const std::variant<Sector, Actor> &clipboard = MapEditor::clipboard.value();
    if (std::holds_alternative<Actor>(clipboard))
    {
        MapEditor::map.actors.push_back(std::get<Actor>(clipboard));
        selectionIndex = MapEditor::map.actors.size() - 1;
        selectionType = ItemType::ACTOR;
    } else if (std::holds_alternative<Sector>(clipboard))
    {
        MapEditor::map.sectors.push_back(std::get<Sector>(clipboard));
        selectionIndex = MapEditor::map.sectors.size() - 1;
        selectionType = ItemType::SECTOR;
    }
}

bool SelectTool::HasSelection()
{
    return selectionType != ItemType::NONE;
}

glm::vec3 SelectTool::SelectionCenter()
{
    if (selectionType == ItemType::ACTOR)
    {
        const Actor &a = MapEditor::map.actors.at(selectionIndex);
        return {a.position[0], a.position[1], a.position[2]};
    }
    if (selectionType == ItemType::SECTOR)
    {
        const Sector &s = MapEditor::map.sectors.at(selectionIndex);
        const std::array<float, 3> center = s.GetCenter();
        return {center[0], center[1], center[2]};
    }
    if (selectionType == ItemType::LINE || selectionType == ItemType::VERTEX)
    {
        const Sector &s = MapEditor::map.sectors.at(focusedSectorIndex);
        const std::array<float, 3> center = s.GetCenter();
        return {center[0], center[1], center[2]};
    }
    if (selectionType == ItemType::CEILING)
    {
        const Sector &s = MapEditor::map.sectors.at(focusedSectorIndex);
        const std::array<float, 3> center = s.GetCenter();
        return {center[0], s.ceilingHeight, center[2]};
    }
    if (selectionType == ItemType::FLOOR)
    {
        const Sector &s = MapEditor::map.sectors.at(focusedSectorIndex);
        const std::array<float, 3> center = s.GetCenter();
        return {center[0], s.floorHeight, center[2]};
    }
    assert(false);
}
