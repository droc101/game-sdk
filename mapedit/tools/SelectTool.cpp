//
// Created by droc101 on 10/3/25.
//

#include "SelectTool.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cfloat>
#include <cstddef>
#include <format>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/Sector.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>
#include <tuple>
#include <variant>
#include <vector>
#include "../EditActorWindow.h"
#include "../MapEditor.h"
#include "../MapRenderer.h"
#include "../Viewport.h"
#include "../ViewportRenderer.h"
#include "EditorTool.h"

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
                actor.position.x = snapped.x;
                actor.position.z = snapped.z;
                dragging = true;
            } else
            {
                dragging = false;
            }
        } else if (selectionType == ItemType::VERTEX &&
                   ((hoverType == ItemType::VERTEX && hoverIndex == selectionVertexIndex) || dragging))
        {
            Sector &sector = MapEditor::map.sectors.at(focusedSectorIndex);
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                const glm::vec3 snapped = MapEditor::SnapToGrid(worldSpaceHover);
                sector.points.at(selectionVertexIndex).x = snapped.x;
                sector.points.at(selectionVertexIndex).y = snapped.z;
                dragging = true;
            } else
            {
                if (!sector.IsValid())
                {
                    sector.points.at(selectionVertexIndex) = vertexDragOriginalPoint;
                }
                dragging = false;
            }
        } else if (selectionType == ItemType::LINE &&
                   ((hoverType == ItemType::LINE && hoverIndex == selectionVertexIndex) || dragging))
        {
            Sector &sector = MapEditor::map.sectors.at(focusedSectorIndex);
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                const glm::vec2 worldHover2D = glm::vec2(worldSpaceHover.x, worldSpaceHover.z);
                const glm::vec2 startPos = worldHover2D - lineDragModeMouseOffset;
                const glm::vec2 endPos = startPos - lineDragModeSecondVertexOffset;
                const glm::vec3 startSnapped = MapEditor::SnapToGrid(glm::vec3(startPos.x, 0, startPos.y));
                const glm::vec3 endSnapped = MapEditor::SnapToGrid(glm::vec3(endPos.x, 0, endPos.y));
                sector.points.at(selectionVertexIndex).x = startSnapped.x;
                sector.points.at(selectionVertexIndex).y = startSnapped.z;
                sector.points.at((selectionVertexIndex + 1) % sector.points.size()).x = endSnapped.x;
                sector.points.at((selectionVertexIndex + 1) % sector.points.size()).y = endSnapped.z;
                dragging = true;
            } else
            {
                if (!sector.IsValid())
                {
                    sector.points.at(selectionVertexIndex) = vertexDragOriginalPoint;
                    sector.points.at((selectionVertexIndex + 1) %
                                     sector.points.size()) = vertexDragOriginalPoint - lineDragModeSecondVertexOffset;
                }
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
                        sector.points.at(0).x,
                        sector.points.at(0).y,
                    };
                    sectorDragMouseOffset = {
                        worldHover2D.x - firstVertex.x,
                        worldHover2D.y - firstVertex.y,
                    };

                    sectorDragVertexOffsets.clear();

                    for (const glm::vec2 &point: MapEditor::map.sectors.at(selectionIndex).points)
                    {
                        sectorDragVertexOffsets.push_back(firstVertex - point);
                    }
                }
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                const glm::vec2 worldHover2D = glm::vec2(worldSpaceHover.x, worldSpaceHover.z);
                const glm::vec2 startPos = worldHover2D - sectorDragMouseOffset;
                for (size_t i = 0; i < sector.points.size(); i++)
                {
                    const glm::vec2 glmPoint = startPos - sectorDragVertexOffsets.at(i);
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
                    actor.position.y = snapped.y;
                    actor.position.z = snapped.z;
                } else
                {
                    actor.position.x = snapped.x;
                    actor.position.y = snapped.y;
                }
                dragging = true;
            } else
            {
                dragging = false;
            }
        } else if (selectionType == ItemType::CEILING && (hoverType == ItemType::CEILING || dragging))
        {
            Sector &sector = MapEditor::map.sectors.at(focusedSectorIndex);
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                const glm::vec3 snapped = MapEditor::SnapToGrid(worldSpaceHover);
                sector.ceilingHeight = snapped.y;
                dragging = true;
            } else
            {
                if (!sector.IsValid())
                {
                    sector.ceilingHeight = vertexDragOriginalPoint.x;
                }
                dragging = false;
            }
        } else if (selectionType == ItemType::FLOOR && (hoverType == ItemType::FLOOR || dragging))
        {
            Sector &sector = MapEditor::map.sectors.at(focusedSectorIndex);
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                const glm::vec3 snapped = MapEditor::SnapToGrid(worldSpaceHover);
                sector.floorHeight = snapped.y;
                dragging = true;
            } else
            {
                if (!sector.IsValid())
                {
                    sector.floorHeight = vertexDragOriginalPoint.y;
                }
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
                vertexDragOriginalPoint = {sector.ceilingHeight, sector.floorHeight};
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
                vertexDragOriginalPoint = {sector.ceilingHeight, sector.floorHeight};
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
                vertexDragOriginalPoint = sector.points.at(vertexIndex);
            } else if (isHovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
                                     ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_RouteGlobal)))
            {
                if (sector.points.size() > 3)
                {
                    sector.points.erase(sector.points.begin() + static_cast<ptrdiff_t>(vertexIndex));
                } else
                {
                    MapEditor::map.sectors.erase(MapEditor::map.sectors.begin() + sectorIndex);
                    selectionType = ItemType::NONE;
                    sectorFocusMode = false;
                }
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
                    vertexDragOriginalPoint = sector.points.at(selectionVertexIndex);
                    haveAddedNewVertex = true;
                } else
                {
                    selectionVertexIndex = vertexIndex;
                    const glm::vec2 startPoint{
                        sector.points.at(vertexIndex).x,
                        sector.points.at(vertexIndex).y,
                    };
                    const glm::vec2 endPoint{
                        sector.points.at((vertexIndex + 1) % sector.points.size()).x,
                        sector.points.at((vertexIndex + 1) % sector.points.size()).y,
                    };
                    const glm::vec2 worldHover2D{worldSpaceHover.x, worldSpaceHover.z};
                    lineDragModeSecondVertexOffset = startPoint - endPoint;
                    lineDragModeMouseOffset = worldHover2D - startPoint;
                    vertexDragOriginalPoint = sector.points.at(selectionVertexIndex);
                    selectionType = ItemType::LINE;
                }
            }
        }
    }
}

std::vector<std::tuple<EditorTool::ItemType, size_t, float>> SelectTool::DetermineHoveredItem(const Viewport &vp,
                                                                                              const bool isHovered,
                                                                                              const glm::vec3
                                                                                                      &worldSpaceHover)
{
    hoverType = ItemType::NONE;
    bool selectionHovered = false;
    std::vector<std::tuple<ItemType, size_t, float>> actorHoverStack{};
    std::vector<std::tuple<ItemType, size_t, float>> sectorHoverStack{};

    for (size_t actorIndex = 0; actorIndex < MapEditor::map.actors.size(); actorIndex++)
    {
        const Actor &a = MapEditor::map.actors.at(actorIndex);
        const glm::vec2 posScreenSpace = vp.WorldToScreenPos(a.position);
        const ImVec2 hoverScreenSpaceIV = vp.GetLocalMousePos();
        const glm::vec2 hoverScreenSpace = glm::vec2(hoverScreenSpaceIV.x, hoverScreenSpaceIV.y);

        if (distance(posScreenSpace, hoverScreenSpace) <= MapEditor::HOVER_DISTANCE_PIXELS && isHovered)
        {
            if (selectionType == ItemType::ACTOR && selectionIndex == actorIndex)
            {
                selectionHovered = true;
                // This actor will be later added with the highest priority
            } else
            {
                if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
                {
                    actorHoverStack.emplace_back(ItemType::ACTOR, actorIndex, a.position.y);
                } else if (vp.GetType() == Viewport::ViewportType::FRONT_XY)
                {
                    actorHoverStack.emplace_back(ItemType::ACTOR, actorIndex, a.position.z);
                } else if (vp.GetType() == Viewport::ViewportType::SIDE_YZ)
                {
                    sectorHoverStack.emplace_back(ItemType::ACTOR, actorIndex, a.position.x);
                }
            }
        }
    }

    if (!actorHoverStack.empty())
    {
        std::ranges::sort(actorHoverStack,
                          [](const std::tuple<ItemType, size_t, float> &a,
                             const std::tuple<ItemType, size_t, float> &b) {
                              return std::get<float>(a) > std::get<float>(b);
                          });
    }

    if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        for (size_t sectorIndex = 0; sectorIndex < MapEditor::map.sectors.size(); sectorIndex++)
        {
            const Sector &sector = MapEditor::map.sectors.at(sectorIndex);
            if (sector.ContainsPoint({worldSpaceHover.x, worldSpaceHover.z}) && isHovered)
            {
                if (selectionType == ItemType::SECTOR && selectionIndex == sectorIndex)
                {
                    selectionHovered = true;
                } else
                {
                    sectorHoverStack.emplace_back(ItemType::SECTOR, sectorIndex, sector.ceilingHeight);
                }
            }
        }

        if (!sectorHoverStack.empty())
        {
            std::ranges::sort(sectorHoverStack,
                              [](const std::tuple<ItemType, size_t, float> &a,
                                 const std::tuple<ItemType, size_t, float> &b) {
                                  return std::get<float>(a) > std::get<float>(b);
                              });
        }
    }

    std::vector<std::tuple<ItemType, size_t, float>> hoverStack{};
    hoverStack.reserve(actorHoverStack.size() + sectorHoverStack.size() + 1);
    if (selectionHovered)
    {
        hoverStack.emplace_back(selectionType, selectionIndex, 0.0f);
    }

    hoverStack.insert(hoverStack.end(), actorHoverStack.begin(), actorHoverStack.end());
    hoverStack.insert(hoverStack.end(), sectorHoverStack.begin(), sectorHoverStack.end());

    if (!hoverStack.empty())
    {
        hoverType = std::get<ItemType>(hoverStack.at(0));
        hoverIndex = std::get<size_t>(hoverStack.at(0));
    }

    return hoverStack;
}

void SelectTool::ProcessViewportSelectMode(const Viewport &vp, const bool isHovered, const glm::vec3 &worldSpaceHover)
{
    const std::vector<std::tuple<ItemType, size_t, float>> &hoverStack = DetermineHoveredItem(vp,
                                                                                              isHovered,
                                                                                              worldSpaceHover);

    if (hoverStack.size() > 1 && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        menuHoveredItems = hoverStack;
        ImGui::OpenPopup("hoverSelectionRClick");
    }

    if (ImGui::BeginPopup("hoverSelectionRClick"))
    {
        for (const std::tuple<ItemType, size_t, float> &item: menuHoveredItems)
        {
            std::string text{};
            const ItemType type = std::get<ItemType>(item);
            size_t index = std::get<size_t>(item);
            if (type == ItemType::ACTOR)
            {
                const Actor &a = MapEditor::map.actors.at(index);
                if (a.params.contains("name") && !a.params.at("name").Get<std::string>("").empty())
                {
                    text = std::format("\"{}\": {}##{}", a.params.at("name").Get<std::string>(""), a.className, index);
                } else
                {
                    text = std::format("{}##{}", a.className, index);
                }
            } else if (type == ItemType::SECTOR)
            {
                const Sector &s = MapEditor::map.sectors.at(index);
                if (s.name.empty())
                {
                    text = std::format("Sector {}", index);
                } else
                {
                    text = std::format("Sector {}: {}", index, s.name);
                }
            } else
            {
                text = "???";
            }
            const bool itemClicked = ImGui::MenuItem(text.c_str());
            if (ImGui::IsItemHovered())
            {
                hoverType = type;
                hoverIndex = index;
            }
            if (itemClicked)
            {
                selectionType = type;
                selectionIndex = index;
                menuHoveredItems.clear();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    for (size_t actorIndex = 0; actorIndex < MapEditor::map.actors.size(); actorIndex++)
    {
        Actor &a = MapEditor::map.actors.at(actorIndex);

        if (hoverType == ItemType::ACTOR && hoverIndex == actorIndex && isHovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            if (ImGui::BeginTooltip())
            {
                if (a.params.contains("name") && !a.params.at("name").Get<std::string>("").empty())
                {
                    ImGui::Text("%s: %s", a.params.at("name").Get<std::string>("").c_str(), a.className.c_str());
                } else
                {
                    ImGui::Text("%s", a.className.c_str());
                }
                ImGui::Text("%.2f, %.2f, %.2f", a.position.x, a.position.y, a.position.z);
                ImGui::EndTooltip();
            }
        }
    }

    for (size_t sectorIndex = 0; sectorIndex < MapEditor::map.sectors.size(); sectorIndex++)
    {
        const Sector &sector = MapEditor::map.sectors.at(sectorIndex);
        if (hoverType == ItemType::SECTOR && hoverIndex == sectorIndex)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            if (ImGui::BeginTooltip())
            {
                if (!sector.name.empty())
                {
                    ImGui::Text("Sector %zu: %s", sectorIndex, sector.name.c_str());
                } else
                {
                    ImGui::Text("Sector %zu", sectorIndex);
                }
                ImGui::EndTooltip();
            }
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

void SelectTool::ProcessViewportVertexMode(Viewport &vp,
                                           glm::mat4 &matrix,
                                           const bool isHovered,
                                           const glm::vec3 &worldSpaceHover,
                                           const glm::vec2 &screenSpaceHover)
{
    hoverType = ItemType::NONE;

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
            const glm::vec2 &start2 = sector.points.at(vertexIndex);
            const glm::vec2 &end2 = sector.points.at((vertexIndex + 1) % sector.points.size());
            const glm::vec3 startCeiling = glm::vec3(start2.x, sector.ceilingHeight, start2.y);
            const glm::vec3 endCeiling = glm::vec3(end2.x, sector.ceilingHeight, end2.y);

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
        }
    }

    for (const Actor &a: MapEditor::map.actors)
    {
        MapRenderer::RenderActor(a, matrix, vp);
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
            } else
            {
                MapEditor::map.sectors.erase(MapEditor::map.sectors.begin() + selectionIndex);
                selectionType = ItemType::NONE;
            }
        }
    }
}


void SelectTool::RenderViewport(Viewport &vp)
{
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
            const ImVec2 localMouse = vp.GetLocalMousePos();
            screenSpaceHover = glm::vec2(localMouse.x, localMouse.y);
        }
    }

    if (sectorFocusMode)
    {
        ProcessViewportVertexMode(vp, matrix, isHovered, worldSpaceHover, screenSpaceHover);
    } else
    {
        ProcessViewportSelectMode(vp, isHovered, worldSpaceHover);
    }

    if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ && selectionType == ItemType::ACTOR)
    {
        EditActorWindow::Render(MapEditor::map.actors.at(selectionIndex));
    }

    const ViewportRenderer::ViewportRenderSettings vps = {
        .sectorFocusMode = sectorFocusMode,
        .focusedSectorIndex = focusedSectorIndex,
        .hoverType = hoverType,
        .hoverIndex = hoverIndex,
        .selectionType = selectionType,
        .selectionIndex = selectionIndex,
        .selectionVertexIndex = selectionVertexIndex,
        .point = nullptr,
        .newPrimitive = nullptr,
        .newActor = nullptr,
    };
    ViewportRenderer::RenderViewport(vp, vps);
}


void SelectTool::RenderToolWindow()
{
    if (!ImGui::CollapsingHeader("Select Tool", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    ImGui::PushItemWidth(-1);
    size_t sectIndex = 0;
    switch (selectionType)
    {
        case ItemType::NONE:
            ImGui::Text("No Selection");
            break;
        case ItemType::VERTEX:
            ImGui::InputFloat2("##vertexPosition",
                               glm::value_ptr(MapEditor::map.sectors.at(focusedSectorIndex)
                                                      .points.at(selectionVertexIndex)));
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

            ImGui::Text("Name");
            ImGui::SameLine();
            ImGui::TextDisabled("(editor only)");
            ImGui::InputText("##sectorName", &MapEditor::map.sectors.at(sectIndex).name);
            break;
        case ItemType::ACTOR:
            ImGui::Text("Position");
            ImGui::InputFloat3("##position", glm::value_ptr(MapEditor::map.actors.at(selectionIndex).position));
            ImGui::Text("Rotation");
            ImGui::InputFloat3("##rotation", glm::value_ptr(MapEditor::map.actors.at(selectionIndex).rotation));
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
        return a.position;
    }
    if (selectionType == ItemType::SECTOR)
    {
        const Sector &s = MapEditor::map.sectors.at(selectionIndex);
        return s.GetCenter();
    }
    if (selectionType == ItemType::LINE || selectionType == ItemType::VERTEX)
    {
        const Sector &s = MapEditor::map.sectors.at(focusedSectorIndex);
        return s.GetCenter();
    }
    if (selectionType == ItemType::CEILING)
    {
        const Sector &s = MapEditor::map.sectors.at(focusedSectorIndex);
        return s.GetCenter();
    }
    if (selectionType == ItemType::FLOOR)
    {
        const Sector &s = MapEditor::map.sectors.at(focusedSectorIndex);
        return s.GetCenter();
    }
    assert(false);
}
