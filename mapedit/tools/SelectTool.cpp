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
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <libassets/type/Actor.h>
#include <libassets/type/Axis.h>
#include <libassets/type/BoundingBox.h>
#include <libassets/type/Brush.h>
#include <memory>
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
#include "FaceTool.h"

SelectTool::SelectTool(const ItemType selectionType, const size_t selectionIndex)
{
    this->selectionType = selectionType;
    this->selectionIndex = selectionIndex;
}

float SelectTool::WrapAndSnapAngle(float angle, float step)
{
    while (angle < 0.0f)
    {
        angle += 360.0f;
    }
    while (angle > 360.0f)
    {
        angle -= 360.0f;
    }
    angle = std::round(angle / step) * step;
    return angle;
}

void SelectTool::HandleDrag(const Viewport &vp, const glm::vec3 worldSpaceHover)
{
    if (vp.Is3D())
    {
        return;
    }

    if (!dragging)
    {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            dragging = true;
            if (selectionType == ItemType::BRUSH)
            {
                const Brush &brush = MapEditor::map.brushes.at(selectionIndex);
                const BoundingBox bbox = brush.GetAABB();
                const ResizeGizmoHelper::Handle handle = ResizeGizmoHelper::HoveredHandle(vp, bbox);
                if (handle != ResizeGizmoHelper::Handle::NONE)
                {
                    draggingResizeGizmo = true;
                    resizeGizmoDragStart = worldSpaceHover;
                    resizeGizmoDraggedHandle = handle;
                    resizeGizmoStartVertices = brush.vertices;
                    resizeGizmoStartBounds = bbox;
                } else
                {
                    draggingResizeGizmo = false;
                }
            } else
            {
                draggingResizeGizmo = false;
            }
            if (!draggingResizeGizmo && (selectionType == ItemType::ACTOR || selectionType == ItemType::BRUSH))
            {
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

                const float dist = glm::distance(vp.GetLocalMousePos(), vp.WorldToScreenPos(objectPosition));
                if (dist >= 44 && dist <= 52) // Nice magic numbers you've got there
                {
                    draggingRotationGizmo = true;

                    const glm::vec2 actorScreenPos = vp.WorldToScreenPos(objectPosition);
                    const glm::vec2 mousePosDifference = vp.GetLocalMousePos() - actorScreenPos;
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
        if (draggingResizeGizmo)
        {
            Brush &brush = MapEditor::map.brushes.at(selectionIndex);
            brush.CenterOrigin();
            draggingResizeGizmo = false;
            return;
        }
    }

    if (draggingResizeGizmo)
    {
        Brush &brush = MapEditor::map.brushes.at(selectionIndex);
        const glm::mat4 transformMatrix = brush.GetTransformMatrix();
        const glm::mat4 inverseTransformMatrix = glm::inverse(transformMatrix);
        const glm::vec3 snappedDrag = MapEditor::SnapToGrid(worldSpaceHover - resizeGizmoDragStart);
        for (size_t i = 0; i < brush.vertices.size(); i++)
        {
            glm::vec3 vertexWorldSpace = transformMatrix * glm::vec4{resizeGizmoStartVertices.at(i), 1};
            ResizeGizmoHelper::ScaleVertex(vertexWorldSpace,
                                           resizeGizmoStartBounds,
                                           snappedDrag,
                                           resizeGizmoDraggedHandle,
                                           vp.GetAxis());
            brush.vertices.at(i) = inverseTransformMatrix * glm::vec4{vertexWorldSpace, 1};
        }
    } else if (draggingRotationGizmo)
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

        float rotationStep = 22.5f;
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
        {
            rotationStep = 5.0f;
        }

        if (selectionType == ItemType::ACTOR)
        {
            Actor &actor = MapEditor::map.actors.at(selectionIndex);
            AxisHelper::SetComponent(vp.GetAxis(),
                                     actor.rotation,
                                     WrapAndSnapAngle(rotationGizmoSelectionAngle, rotationStep));
        } else if (selectionType == ItemType::BRUSH)
        {
            Brush &brush = MapEditor::map.brushes.at(selectionIndex);
            AxisHelper::SetComponent(vp.GetAxis(),
                                     brush.rotation,
                                     WrapAndSnapAngle(rotationGizmoSelectionAngle, rotationStep));
        }
    } else if (selectionType == ItemType::ACTOR)
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
                                                                                              const glm::vec3
                                                                                                      &worldSpaceHover)
{
    if (vp.Is3D())
    {
        hoverType = ItemType::NONE;
        const Viewport::PerspectiveHoverInfo hoverInfo = vp.GetPerspectiveHover();
        std::vector<std::tuple<ItemType, size_t, float>> stack{};
        if (hoverInfo.objectType != ItemType::NONE)
        {
            stack.emplace_back(static_cast<ItemType>(hoverInfo.objectType), hoverInfo.objectIndex, 0.0f);
            hoverType = static_cast<ItemType>(hoverInfo.objectType);
            hoverIndex = hoverInfo.objectIndex;
        }
        return stack;
    }

    hoverType = ItemType::NONE;
    bool selectionHovered = false;
    std::vector<std::tuple<ItemType, size_t, float>> actorHoverStack{};
    std::vector<std::tuple<ItemType, size_t, float>> sectorHoverStack{};

    for (size_t actorIndex = 0; actorIndex < MapEditor::map.actors.size(); actorIndex++)
    {
        const Actor &a = MapEditor::map.actors.at(actorIndex);
        const glm::vec2 posScreenSpace = vp.WorldToScreenPos(a.position);

        if (glm::distance(posScreenSpace, vp.GetLocalMousePos()) <= MapEditor::HOVER_DISTANCE_PIXELS)
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
            if (brush.ContainsPoint(vp.GetAxis(), vp.Make2D(worldSpaceHover)))
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
        const float dist = glm::distance(vp.GetLocalMousePos(), vp.WorldToScreenPos(actor.position));
        if (dist >= 44 && dist <= 52)
        {
            hoverStack.emplace_back(selectionType, selectionIndex, 0.0f);
        }
    } else if (selectionType == ItemType::BRUSH)
    {
        const Brush &brush = MapEditor::map.brushes.at(selectionIndex);
        const float dist = glm::distance(vp.GetLocalMousePos(), vp.WorldToScreenPos(brush.origin));

        bool hoveringRotation = dist >= 44 && dist <= 52;
        bool hoveringResize = ResizeGizmoHelper::HoveredHandle(vp, brush.GetAABB()) != ResizeGizmoHelper::Handle::NONE;
        if (hoveringRotation || hoveringResize)
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

void SelectTool::ProcessViewportSelectMode(const Viewport &vp, const glm::vec3 &worldSpaceHover)
{
    const std::vector<std::tuple<ItemType, size_t, float>> &hoverStack = DetermineHoveredItem(vp, worldSpaceHover);

    const ImGuiMouseCursor hoverCursor = vp.Is3D() ? ImGuiMouseCursor_Hand : ImGuiMouseCursor_ResizeAll;

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

        if (hoverType == ItemType::ACTOR && hoverIndex == actorIndex)
        {
            ImGui::SetMouseCursor(hoverCursor);
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
            ImGui::SetMouseCursor(hoverCursor);
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
    } else if (vp.Is3D() && selectionType == ItemType::BRUSH && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        const Viewport::PerspectiveHoverInfo hoverInfo = vp.GetPerspectiveHover();
        if (hoverInfo.objectType == ItemType::BRUSH && hoverInfo.objectIndex == selectionIndex)
        {
            MapEditor::toolType = MapEditor::EditorToolType::FACE;
            MapEditor::tool = std::make_unique<FaceTool>(hoverInfo.objectIndex, hoverInfo.objectSubIndex);
        }
    }

    HandleDrag(vp, worldSpaceHover);

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
    const bool isFocusedAndHovered = ImGui::IsWindowFocused() && ImGui::IsWindowHovered();
    if (isFocusedAndHovered)
    {
        ProcessViewportSelectMode(vp, vp.GetWorldSpaceMousePos());
    }

    bool showRotationGizmo = false;
    ViewportRenderer::ViewportRenderRotationGizmo rotationGizmo = {
        .position = glm::vec3(0),
        .radiusInPx = 48,
    };

    bool showResizeGizmo = false;
    ViewportRenderer::ViewportRenderResizeGizmo resizeGizmo = {
        .hoveredHandle = ResizeGizmoHelper::Handle::NONE,
        .boundingBox = BoundingBox(),
    };

    if (selectionType == ItemType::ACTOR)
    {
        rotationGizmo.position = MapEditor::map.actors.at(selectionIndex).position;
        showRotationGizmo = true;
    } else if (selectionType == ItemType::BRUSH)
    {
        const Brush &brush = MapEditor::map.brushes.at(selectionIndex);
        rotationGizmo.position = brush.origin;
        const BoundingBox bb = brush.GetAABB();
        resizeGizmo.hoveredHandle = ResizeGizmoHelper::HoveredHandle(vp, bb);
        resizeGizmo.boundingBox = bb;
        showRotationGizmo = true;
        showResizeGizmo = true;
    }

    const ViewportRenderer::ViewportRenderSettings vps = {
        .hoverType = hoverType,
        .hoverIndex = hoverIndex,
        .selectionType = selectionType,
        .selectionIndex = selectionIndex,
        .renderObjectBuffers = isFocusedAndHovered,
        .point = nullptr,
        .newActor = nullptr,
        .rotationGizmo = showRotationGizmo ? &rotationGizmo : nullptr,
        .resizeGizmo = showResizeGizmo ? &resizeGizmo : nullptr,
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
        {
            Actor &actor = MapEditor::map.actors.at(selectionIndex);
            if (actor.params.contains("name") &&
                actor.params.at("name").GetType() == Param::ParamType::PARAM_TYPE_STRING &&
                !actor.params.at("name").Get<std::string>("").empty())
            {
                ImGui::Text("%s", actor.params.at("name").Get<std::string>("").c_str());
                ImGui::SameLine();
                ImGui::TextDisabled(": %s", actor.className.c_str());
            } else
            {
                ImGui::Text("%s", actor.className.c_str());
            }
            ImGui::Separator();
            ImGui::Text("Position");
            ImGui::InputFloat3("##position", glm::value_ptr(actor.position));
            ImGui::Text("Rotation");
            ImGui::InputFloat3("##rotation", glm::value_ptr(actor.rotation));
            ImGui::Separator();
            if (ImGui::Button("Actor Properties", {-1, 0}))
            {
                WindowManager::Get().AddModalWindow<EditActorWindow>(actor);
            }
            break;
        }
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
#ifdef NDEBUG
    return glm::vec3{0};
#endif
}
