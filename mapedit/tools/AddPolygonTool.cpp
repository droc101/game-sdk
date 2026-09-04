//
// Created by droc101 on 9/19/25.
//

#include "AddPolygonTool.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <game_sdk/WindowManager.h>
#include <imgui.h>
#include <libassets/type/Axis.h>
#include <libassets/type/BoundingBox.h>
#include <libassets/type/Brush.h>
#include <libassets/type/Color.h>
#include <memory>
#include <vector>
#include "../MapEditor.h"
#include "../Viewport.h"
#include "../ViewportRenderer.h"
#include "EditorTool.h"
#include "SelectTool.h"

void AddPolygonTool::AddBrush()
{
    if (shapeStart == shapeEnd || startDepth == endDepth)
    {
        return;
    }

    Brush b = Brush();
    for (const glm::vec2 &glmPoint: points)
    {
        b.vertices.push_back(AxisHelper::Make3D(axis, glmPoint, startDepth));
    }
    for (const glm::vec2 &glmPoint: points)
    {
        b.vertices.push_back(AxisHelper::Make3D(axis, glmPoint, endDepth));
    }

    for (size_t i = 0; i < points.size(); i++)
    {
        Brush::Face face{};
        face.material = MapEditor::material;
        size_t nextIndex = (i + 1) % points.size();
        face.indices.push_back(i);
        face.indices.push_back(nextIndex);
        face.indices.push_back(nextIndex + points.size());
        face.indices.push_back(i + points.size());
        b.faces.push_back(face);
    }

    for (uint8_t whichCap = 0; whichCap < 2; whichCap++)
    {
        Brush::Face face{};
        face.material = MapEditor::material;
        for (size_t i = 0; i < points.size(); i++)
        {
            face.indices.push_back(i + (whichCap == 1 ? points.size() : 0));
        }
        b.faces.push_back(face);
    }

    b.CenterOrigin(MapEditor::GRID_SPACING_VALUES[MapEditor::gridSpacingIndex]);

    if (!b.IsValid())
    {
        WindowManager::Get().GetCurrentWindow()->ErrorMessage("Brush has invalid shape and will not "
                                                              "be added");
    } else
    {
        MapEditor::map.brushes.push_back(b);
    }
    state = PolygonToolState::IDLE;
}

void AddPolygonTool::RenderToolWindow()
{
    if (!ImGui::CollapsingHeader("Polygon Tool", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    ImGui::PushItemWidth(-1);
    MapEditor::MaterialSelectionTool(MapEditor::material);
}

void AddPolygonTool::RenderViewport(Viewport &vp)
{
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

    if (ImGui::IsWindowFocused() && !vp.Is3D())
    {
        if (state == PolygonToolState::IDLE)
        {
            const glm::vec2 pt = MapEditor::SnapToGrid(vp.Make2D(worldSpaceHover));
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                points = {pt};
                state = PolygonToolState::DRAWING;
                axis = vp.GetAxis();
            }

            if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
            {
                MapEditor::toolType = MapEditor::EditorToolType::SELECT;
                MapEditor::tool = std::make_unique<SelectTool>();
                return;
            }
        } else if (state == PolygonToolState::DRAWING)
        {
            if (ImGui::Shortcut(ImGuiKey_Escape))
            {
                state = PolygonToolState::IDLE;
            }

            if (vp.GetAxis() == axis)
            {
                const glm::vec2 screenSpaceFirstPoint = vp.WorldToScreenPos(AxisHelper::Make3D(axis,
                                                                                              points.at(0),
                                                                                              startDepth));
                if (distance(screenSpaceHover, screenSpaceFirstPoint) < 5 ||
                    AxisHelper::Make2D(axis, worldSpaceHover) == points.at(0))
                {
                    if (points.size() < 3)
                    {
                        if (ImGui::BeginTooltip())
                        {
                            ImGui::Text("Cannot create brush with less than 3 points");
                            ImGui::EndTooltip();
                        }
                    } else
                    {
                        if (ImGui::BeginTooltip())
                        {
                            ImGui::Text("Close Brush");
                            ImGui::EndTooltip();
                        }
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            std::vector<glm::vec3> points3d{};
                            for (const glm::vec2 &point: points)
                            {
                                points3d.push_back(AxisHelper::Make3D(axis, point, startDepth));
                                points3d.push_back(AxisHelper::Make3D(axis, point, endDepth));
                            }
                            const BoundingBox bb = BoundingBox(points3d);
                            shapeStart = AxisHelper::Make2D(axis, bb.StartPosition());
                            shapeEnd = AxisHelper::Make2D(axis, bb.EndPosition());
                            state = PolygonToolState::WAITING_TO_PLACE;
                        }
                    }
                } else
                {
                    const glm::vec2 worldSpacePoint = AxisHelper::Make2D(axis, worldSpaceHover);
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        const glm::vec2 point = MapEditor::SnapToGrid(worldSpacePoint);
                        if (std::ranges::find(points, point) == points.end())
                        {
                            points.push_back(point);
                        }
                    }
                }
            }
        } else if (state == PolygonToolState::WAITING_TO_PLACE)
        {
            if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
            {
                state = PolygonToolState::IDLE;
            }
            if (ImGui::Shortcut(ImGuiKey_Enter, ImGuiInputFlags_RouteGlobal))
            {
                AddBrush();
            }
            if (vp.GetAxis() != axis && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                const glm::vec2 twoDimHover = vp.Make2D(worldSpaceHover);

                const glm::vec2 startDepthA = vp.Make2D(AxisHelper::Make3D(axis, shapeStart, startDepth));
                const glm::vec2 startDepthB = vp.Make2D(AxisHelper::Make3D(axis, shapeEnd, startDepth));
                if (MapEditor::VecDistanceToLine2D(startDepthA, startDepthB, twoDimHover) <=
                    MapEditor::HOVER_DISTANCE_PIXELS)
                {
                    state = PolygonToolState::DRAGGING_START_DEPTH;
                }

                const glm::vec2 endDepthA = vp.Make2D(AxisHelper::Make3D(axis, shapeStart, endDepth));
                const glm::vec2 endDepthB = vp.Make2D(AxisHelper::Make3D(axis, shapeEnd, endDepth));
                if (MapEditor::VecDistanceToLine2D(endDepthA, endDepthB, twoDimHover) <=
                    MapEditor::HOVER_DISTANCE_PIXELS)
                {
                    state = PolygonToolState::DRAGGING_END_DEPTH;
                }
            }
        } else if (state == PolygonToolState::DRAGGING_START_DEPTH)
        {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                state = PolygonToolState::WAITING_TO_PLACE;
            }
            switch (axis)
            {
                case Axis::X:
                    startDepth = MapEditor::SnapToGrid(worldSpaceHover.x);
                    break;
                case Axis::Y:
                    startDepth = MapEditor::SnapToGrid(worldSpaceHover.y);
                    break;
                case Axis::Z:
                    startDepth = MapEditor::SnapToGrid(worldSpaceHover.z);
                    break;
            }
        } else if (state == PolygonToolState::DRAGGING_END_DEPTH)
        {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                state = PolygonToolState::WAITING_TO_PLACE;
            }
            switch (axis)
            {
                case Axis::X:
                    endDepth = MapEditor::SnapToGrid(worldSpaceHover.x);
                    break;
                case Axis::Y:
                    endDepth = MapEditor::SnapToGrid(worldSpaceHover.y);
                    break;
                case Axis::Z:
                    endDepth = MapEditor::SnapToGrid(worldSpaceHover.z);
                    break;
            }
        }
    }

    ViewportRenderer::ViewportRenderPoint vpt{};
    bool showPoint = false;
    if (!vp.Is3D() &&
        ImGui::IsWindowFocused() &&
        (state == PolygonToolState::IDLE || state == PolygonToolState::DRAWING))
    {
        const glm::vec3 pt = MapEditor::SnapToGrid(worldSpaceHover) + vp.Make3D(glm::vec2(0), 0.1);
        vpt = {
            .pos = pt,
            .color = Color(1, 0.7, 0.7, 1),
            .size = 10,
        };
        showPoint = true;
    }

    ViewportRenderer::ViewportRenderNewPolygon sect = {
        .points = points,
        .startDepth = startDepth,
        .endDepth = endDepth,
        .axis = axis,
    };

    ViewportRenderer::ViewportRenderNewPrimitive newPrim = {
        .points = points,
        .startDepth = startDepth,
        .endDepth = endDepth,
        .aabbStart = shapeStart,
        .aabbEnd = shapeEnd,
        .axis = axis,
    };
    const bool showNewPrim = state != PolygonToolState::IDLE && state != PolygonToolState::DRAWING;

    const ViewportRenderer::ViewportRenderSettings vps = {
        .brushFocusMode = false,
        .focusedBrushIndex = 0,
        .hoverType = ItemType::NONE,
        .hoverIndex = 0,
        .selectionType = ItemType::NONE,
        .selectionIndex = 0,
        .selectionVertexIndex = 0,
        .point = showPoint ? &vpt : nullptr,
        .newPrimitive = showNewPrim ? &newPrim : nullptr,
        .newPolygon = state == PolygonToolState::DRAWING ? &sect : nullptr,
    };
    ViewportRenderer::RenderViewport(vp, vps);
}
