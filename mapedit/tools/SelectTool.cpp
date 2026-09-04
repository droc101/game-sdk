//
// Created by droc101 on 10/3/25.
//

#include "SelectTool.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <format>
#include <game_sdk/WindowManager.h>
#include <glm/gtc/round.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <libassets/type/Actor.h>
#include <libassets/type/Axis.h>
#include <libassets/type/BoundingBox.h>
#include <libassets/type/Brush.h>
#include <misc/cpp/imgui_stdlib.h>
#include <numbers>
#include <string>
#include <tuple>
#include <variant>
#include <vector>
#include "../EditActorWindow.h"
#include "../MapEditor.h"
#include "../Viewport.h"
#include "../ViewportRenderer.h"
#include "EditorTool.h"

float SelectTool::WrapAndSnapAngle(float angle)
{
    while (angle < 0.0f)
    {
        angle += 360.0f;
    }
    while (angle > 360.0f)
    {
        angle -= 360.0f;
    }
    angle = std::round(angle / 22.5f) * 22.5f;
    return angle;
}

void SelectTool::HandleDrag(const Viewport &vp, const bool isHovered, const glm::vec3 worldSpaceHover)
{
    if (!isHovered)
    {
        return;
    }

    if (!dragging)
    {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            dragging = true;
            if (selectionType == ItemType::ACTOR || selectionType == ItemType::BRUSH)
            {
                const ImVec2 lmp = vp.GetLocalMousePos();

                glm::vec3 objectPosition;
                glm::vec3 objectRotation;
                if (selectionType == ItemType::ACTOR)
                {
                    const Actor &actor = MapEditor::map.actors.at(selectionIndex);
                    objectPosition = actor.position;
                    objectRotation = actor.rotation;
                } else if (selectionType == ItemType::BRUSH)
                {
                    const Brush &brush = MapEditor::map.brushes.at(selectionIndex);
                    objectPosition = brush.origin;
                    objectRotation = brush.rotation;
                }

                const float dist = glm::distance({lmp.x, lmp.y}, vp.WorldToScreenPos(objectPosition));

                if (dist >= 44 && dist <= 52)
                {
                    draggingRotationGizmo = true;

                    const glm::vec2 actorScreenPos = vp.WorldToScreenPos(objectPosition);
                    const glm::vec2 mousePos = {vp.GetLocalMousePos().x, vp.GetLocalMousePos().y};
                    const glm::vec2 mousePosDifference = mousePos - actorScreenPos;
                    rotationGizmoLastAngle = atanf(mousePosDifference.x / mousePosDifference.y);
                    if (mousePosDifference.y > 0)
                    {
                        rotationGizmoLastAngle += std::numbers::pi_v<float>;
                    }
                    switch (vp.GetAxis())
                    {
                        case Axis::Y:
                            rotationGizmoSelectionAngle = objectRotation.y;
                            break;
                        case Axis::Z:
                            rotationGizmoSelectionAngle = objectRotation.z;
                            break;
                        case Axis::X:
                            rotationGizmoSelectionAngle = objectRotation.x;
                            break;
                    }
                } else
                {
                    if (selectionType == ItemType::BRUSH)
                    {
                        const Brush &brush = MapEditor::map.brushes.at(selectionIndex);
                        brushDragMouseOffset = MapEditor::SnapToGrid(vp.Make2D(brush.origin - worldSpaceHover));
                    }
                    draggingRotationGizmo = false;
                }
            } else
            {
                draggingRotationGizmo = false;
            }
        }
        return;
    }

    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        dragging = false;
    }

    if (draggingRotationGizmo)
    {
        const glm::vec2 mousePos = {vp.GetLocalMousePos().x, vp.GetLocalMousePos().y};

        glm::vec2 actorScreenPos;

        if (selectionType == ItemType::ACTOR)
        {
            const Actor &actor = MapEditor::map.actors.at(selectionIndex);
            actorScreenPos = vp.WorldToScreenPos(actor.position);
        } else if (selectionType == ItemType::BRUSH)
        {
            const Brush &brush = MapEditor::map.brushes.at(selectionIndex);
            actorScreenPos = vp.WorldToScreenPos(brush.origin);
        }

        const glm::vec2 mousePosDifference = mousePos - actorScreenPos;
        float newAngle = atanf(mousePosDifference.x / mousePosDifference.y);
        if (mousePosDifference.y > 0)
        {
            newAngle += std::numbers::pi_v<float>;
        }
        const float angleDiff = glm::degrees(newAngle - rotationGizmoLastAngle);
        rotationGizmoLastAngle = newAngle;
        rotationGizmoSelectionAngle += angleDiff;

        if (selectionType == ItemType::ACTOR)
        {
            Actor &actor = MapEditor::map.actors.at(selectionIndex);
            AxisHelper::SetComponent(vp.GetAxis(), actor.rotation, WrapAndSnapAngle(rotationGizmoSelectionAngle));
        } else if (selectionType == ItemType::BRUSH)
        {
            Brush &brush = MapEditor::map.brushes.at(selectionIndex);
            AxisHelper::SetComponent(vp.GetAxis(), brush.rotation, WrapAndSnapAngle(rotationGizmoSelectionAngle));
        }

        return;
    }

    if (selectionType == ItemType::ACTOR)
    {
        Actor &actor = MapEditor::map.actors.at(selectionIndex);
        const glm::vec3 snapped = MapEditor::SnapToGrid(worldSpaceHover);
        AxisHelper::Set2DComponents(vp.GetAxis(), actor.position, vp.Make2D(snapped));
    } else if (selectionType == ItemType::BRUSH)
    {
        Brush &brush = MapEditor::map.brushes.at(selectionIndex);
        const glm::vec3 snapped = MapEditor::SnapToGrid(worldSpaceHover);
        AxisHelper::Set2DComponents(vp.GetAxis(), brush.origin, vp.Make2D(snapped) + brushDragMouseOffset);
    }
}

std::vector<std::tuple<EditorTool::ItemType, size_t, float>> SelectTool::DetermineHoveredItem(const Viewport &vp,
                                                                                              const bool isHovered,
                                                                                              const glm::vec3
                                                                                                      &worldSpaceHover)
{
    const ImVec2 localMousePos = vp.GetLocalMousePos();

    hoverType = ItemType::NONE;
    bool selectionHovered = false;
    std::vector<std::tuple<ItemType, size_t, float>> actorHoverStack{};
    std::vector<std::tuple<ItemType, size_t, float>> sectorHoverStack{};

    for (size_t actorIndex = 0; actorIndex < MapEditor::map.actors.size(); actorIndex++)
    {
        const Actor &a = MapEditor::map.actors.at(actorIndex);
        const glm::vec2 posScreenSpace = vp.WorldToScreenPos(a.position);

        const glm::vec2 hoverScreenSpace = glm::vec2(localMousePos.x, localMousePos.y);

        if (distance(posScreenSpace, hoverScreenSpace) <= MapEditor::HOVER_DISTANCE_PIXELS && isHovered)
        {
            if (selectionType == ItemType::ACTOR && selectionIndex == actorIndex)
            {
                selectionHovered = true;
                // This actor will be later added with the highest priority
            } else
            {
                actorHoverStack.emplace_back(ItemType::ACTOR,
                                             actorIndex,
                                             AxisHelper::GetComponent(vp.GetAxis(), a.position));
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

    if (!vp.Is3D())
    {
        for (size_t brushIndex = 0; brushIndex < MapEditor::map.brushes.size(); brushIndex++)
        {
            const Brush &brush = MapEditor::map.brushes.at(brushIndex);
            if (brush.ContainsPoint(vp.GetAxis(), vp.Make2D(worldSpaceHover)) && isHovered)
            {
                if (selectionType == ItemType::BRUSH && selectionIndex == brushIndex)
                {
                    selectionHovered = true;
                } else
                {
                    const BoundingBox bb = brush.GetAABB();
                    const float componentA = AxisHelper::GetComponent(vp.GetAxis(), bb.StartPosition());
                    const float componentB = AxisHelper::GetComponent(vp.GetAxis(), bb.EndPosition());
                    sectorHoverStack.emplace_back(ItemType::BRUSH, brushIndex, std::max(componentA, componentB));
                }
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

    std::vector<std::tuple<ItemType, size_t, float>> hoverStack{};

    if (dragging)
    {
        hoverStack.emplace_back(selectionType, selectionIndex, 0.0f);
        return hoverStack;
    }


    hoverStack.reserve(actorHoverStack.size() + sectorHoverStack.size() + 1);
    if (selectionHovered)
    {
        hoverStack.emplace_back(selectionType, selectionIndex, 0.0f);
    }


    if (selectionType == ItemType::ACTOR)
    {
        const Actor &actor = MapEditor::map.actors.at(selectionIndex);
        const float dist = glm::distance({localMousePos.x, localMousePos.y}, vp.WorldToScreenPos(actor.position));
        if (dist >= 44 && dist <= 52)
        {
            hoverStack.emplace_back(selectionType, selectionIndex, 0.0f);
        }
    } else if (selectionType == ItemType::BRUSH)
    {
        const Brush &brush = MapEditor::map.brushes.at(selectionIndex);
        const float dist = glm::distance({localMousePos.x, localMousePos.y}, vp.WorldToScreenPos(brush.origin));
        if (dist >= 44 && dist <= 52)
        {
            hoverStack.emplace_back(selectionType, selectionIndex, 0.0f);
        }
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
            } else if (type == ItemType::BRUSH)
            {
                const Brush &b = MapEditor::map.brushes.at(index);
                if (b.editorName.empty())
                {
                    text = std::format("Brush {}", index);
                } else
                {
                    text = std::format("Brush {}: {}", index, b.editorName);
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

    for (size_t brushIndex = 0; brushIndex < MapEditor::map.brushes.size(); brushIndex++)
    {
        const Brush &brush = MapEditor::map.brushes.at(brushIndex);
        if (hoverType == ItemType::BRUSH && hoverIndex == brushIndex)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            if (ImGui::BeginTooltip())
            {
                if (!brush.editorName.empty())
                {
                    ImGui::Text("Brush %zu: %s", brushIndex, brush.editorName.c_str());
                } else
                {
                    ImGui::Text("Brush %zu", brushIndex);
                }
                ImGui::EndTooltip();
            }
        }
    }

    if (isHovered)
    {
        if ((hoverType == ItemType::NONE && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) ||
            ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
        {
            selectionType = ItemType::NONE;
        } else if (hoverType == ItemType::BRUSH && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            selectionIndex = hoverIndex;
            selectionType = ItemType::BRUSH;
        } else if (hoverType == ItemType::ACTOR && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            selectionIndex = hoverIndex;
            selectionType = ItemType::ACTOR;
        }

        if (selectionType == ItemType::ACTOR && ImGui::Shortcut(ImGuiMod_Alt | ImGuiKey_Enter))
        {
            Actor &toEdit = MapEditor::map.actors.at(selectionIndex);
            WindowManager::Get().AddModalWindow<EditActorWindow>(toEdit);
        }
    }

    HandleDrag(vp, isHovered, worldSpaceHover);

    if (ImGui::Shortcut(ImGuiKey_Delete))
    {
        if (selectionType == ItemType::ACTOR)
        {
            MapEditor::map.actors.erase(MapEditor::map.actors.begin() + selectionIndex);
            if (hoverType == ItemType::ACTOR && hoverIndex == selectionIndex)
            {
                hoverType = ItemType::NONE;
            }
            selectionType = ItemType::NONE;
        } else if (selectionType == ItemType::BRUSH)
        {
            MapEditor::map.brushes.erase(MapEditor::map.brushes.begin() + selectionIndex);
            if (hoverType == ItemType::BRUSH && hoverIndex == selectionIndex)
            {
                hoverType = ItemType::NONE;
            }
            selectionType = ItemType::NONE;
        }
    }
}

void SelectTool::RenderViewport(Viewport &vp)
{
    bool isHovered = false;
    glm::vec3 worldSpaceHover{};

    if (ImGui::IsWindowFocused())
    {
        isHovered = ImGui::IsWindowHovered();
        if (isHovered)
        {
            worldSpaceHover = vp.GetWorldSpaceMousePos();
        }
    }

    ProcessViewportSelectMode(vp, isHovered, worldSpaceHover);

    bool showGizmo = false;
    ViewportRenderer::ViewportRenderGizmo gizmo = {
        .position = glm::vec3(0),
        .radiusInPx = 48,
    };
    if (selectionType == ItemType::ACTOR)
    {
        gizmo.position = MapEditor::map.actors.at(selectionIndex).position;
        showGizmo = true;
    } else if (selectionType == ItemType::BRUSH)
    {
        gizmo.position = MapEditor::map.brushes.at(selectionIndex).origin;
        showGizmo = true;
    }

    const ViewportRenderer::ViewportRenderSettings vps = {
        .hoverType = hoverType,
        .hoverIndex = hoverIndex,
        .selectionType = selectionType,
        .selectionIndex = selectionIndex,
        .point = nullptr,
        .newActor = nullptr,
        .gizmo = showGizmo ? &gizmo : nullptr,
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
    switch (selectionType)
    {
        case ItemType::NONE:
            ImGui::Text("No Selection");
            break;
        case ItemType::BRUSH:
            ImGui::Text("Name");
            ImGui::SameLine();
            ImGui::TextDisabled("(editor only)");
            ImGui::InputText("##brushName", &MapEditor::map.brushes.at(selectionIndex).editorName);
            ImGui::Separator();
            ImGui::Text("Position");
            ImGui::InputFloat3("##position", glm::value_ptr(MapEditor::map.brushes.at(selectionIndex).origin));
            ImGui::Text("Rotation");
            ImGui::InputFloat3("##rotation", glm::value_ptr(MapEditor::map.brushes.at(selectionIndex).rotation));
            break;
        case ItemType::ACTOR:
            ImGui::Text("Position");
            ImGui::InputFloat3("##position", glm::value_ptr(MapEditor::map.actors.at(selectionIndex).position));
            ImGui::Text("Rotation");
            ImGui::InputFloat3("##rotation", glm::value_ptr(MapEditor::map.actors.at(selectionIndex).rotation));
            ImGui::Separator();
            if (ImGui::Button("Actor Properties"))
            {
                Actor &toEdit = MapEditor::map.actors.at(selectionIndex);
                WindowManager::Get().AddModalWindow<EditActorWindow>(toEdit);
            }
            break;
        default:
            ImGui::Text("The current selection has no properties");
    }
}

bool SelectTool::IsCopyableSelected() const
{
    return selectionType == ItemType::ACTOR || selectionType == ItemType::BRUSH;
}

void SelectTool::Copy() const
{
    if (selectionType == ItemType::ACTOR)
    {
        MapEditor::clipboard = MapEditor::map.actors.at(selectionIndex);
    } else if (selectionType == ItemType::BRUSH)
    {
        MapEditor::clipboard = MapEditor::map.brushes.at(selectionIndex);
    }
}

void SelectTool::Cut()
{
    Copy();
    if (selectionType == ItemType::ACTOR)
    {
        MapEditor::map.actors.erase(MapEditor::map.actors.begin() + selectionIndex);
    } else if (selectionType == ItemType::BRUSH)
    {
        MapEditor::map.brushes.erase(MapEditor::map.brushes.begin() + selectionIndex);
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
    const std::variant<Brush, Actor> &clipboard = MapEditor::clipboard.value();
    if (std::holds_alternative<Actor>(clipboard))
    {
        MapEditor::map.actors.push_back(std::get<Actor>(clipboard));
        selectionIndex = MapEditor::map.actors.size() - 1;
        selectionType = ItemType::ACTOR;
    } else if (std::holds_alternative<Brush>(clipboard))
    {
        MapEditor::map.brushes.push_back(std::get<Brush>(clipboard));
        selectionIndex = MapEditor::map.brushes.size() - 1;
        selectionType = ItemType::BRUSH;
    }
}

bool SelectTool::HasSelection() const
{
    return selectionType != ItemType::NONE;
}

glm::vec3 SelectTool::SelectionCenter() const
{
    if (selectionType == ItemType::ACTOR)
    {
        const Actor &a = MapEditor::map.actors.at(selectionIndex);
        return a.position;
    }
    if (selectionType == ItemType::BRUSH)
    {
        const Brush &b = MapEditor::map.brushes.at(selectionIndex);
        return b.origin;
    }
    assert(false);
}
