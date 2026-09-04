//
// Created by droc101 on 9/21/25.
//

#include "AddPrimitiveTool.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <game_sdk/WindowManager.h>
#include <imgui.h>
#include <libassets/type/Axis.h>
#include <libassets/type/Brush.h>
#include <memory>
#include <numbers>
#include <vector>
#include "../MapEditor.h"
#include "../Viewport.h"
#include "../ViewportRenderer.h"
#include "EditorTool.h"
#include "SelectTool.h"

void AddPrimitiveTool::AddBrush()
{
    if (shapeStart == shapeEnd || startDepth == endDepth)
    {
        return;
    }

    Brush b = Brush();
    const std::vector<glm::vec2> points = GetPoints();
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
    hasDrawnShape = false;
    dragMode = DragMode::NOT_DRAGGING;
}

void AddPrimitiveTool::RenderViewport(Viewport &vp)
{
    glm::vec3 worldSpaceHover{};

    if (ImGui::IsWindowFocused() && !vp.Is3D())
    {
        if (ImGui::IsWindowHovered())
        {
            worldSpaceHover = vp.GetWorldSpaceMousePos();
        }

        if (dragMode == DragMode::NOT_DRAGGING && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if (hasDrawnShape && vp.GetAxis() != axis)
            {
                const glm::vec2 twoDimHover = vp.Make2D(worldSpaceHover);

                const glm::vec2 startDepthA = vp.Make2D(AxisHelper::Make3D(axis, shapeStart, startDepth));
                const glm::vec2 startDepthB = vp.Make2D(AxisHelper::Make3D(axis, shapeEnd, startDepth));
                if (MapEditor::VecDistanceToLine2D(startDepthA, startDepthB, twoDimHover) <=
                    MapEditor::HOVER_DISTANCE_PIXELS)
                {
                    dragMode = DragMode::DRAGGING_START_DEPTH;
                }

                const glm::vec2 endDepthA = vp.Make2D(AxisHelper::Make3D(axis, shapeStart, endDepth));
                const glm::vec2 endDepthB = vp.Make2D(AxisHelper::Make3D(axis, shapeEnd, endDepth));
                if (MapEditor::VecDistanceToLine2D(endDepthA, endDepthB, twoDimHover) <=
                    MapEditor::HOVER_DISTANCE_PIXELS)
                {
                    dragMode = DragMode::DRAGGING_END_DEPTH;
                }
            }
            if (dragMode == DragMode::NOT_DRAGGING)
            {
                shapeStart = vp.Make2D(MapEditor::SnapToGrid(worldSpaceHover));
                shapeEnd = vp.Make2D(MapEditor::SnapToGrid(worldSpaceHover));
                dragMode = DragMode::DRAGGING_PRIMARY_BOX;
                hasDrawnShape = true;
                axis = vp.GetAxis();
            }
        } else if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            switch (dragMode)
            {
                case DragMode::DRAGGING_PRIMARY_BOX:
                    shapeEnd = vp.Make2D(MapEditor::SnapToGrid(worldSpaceHover));
                    break;
                case DragMode::DRAGGING_START_DEPTH:
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
                    break;
                case DragMode::DRAGGING_END_DEPTH:
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
                    break;
                default:
                    break;
            }
        } else
        {
            if (shapeStart == shapeEnd || startDepth == endDepth)
            {
                hasDrawnShape = false;
            }
            dragMode = DragMode::NOT_DRAGGING;
        }
    }

    if (hasDrawnShape)
    {
        if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
        {
            hasDrawnShape = false;
        }
        if (ImGui::Shortcut(ImGuiKey_Enter, ImGuiInputFlags_RouteGlobal))
        {
            AddBrush();
        }
    } else if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
    {
        MapEditor::toolType = MapEditor::EditorToolType::SELECT;
        MapEditor::tool = std::make_unique<SelectTool>();
    }

    ViewportRenderer::ViewportRenderNewPrimitive newPrim = {
        .points = GetPoints(),
        .startDepth = startDepth,
        .endDepth = endDepth,
        .aabbStart = shapeStart,
        .aabbEnd = shapeEnd,
        .axis = axis,
    };

    ViewportRenderer::ViewportRenderPoint vpt{};
    bool showPoint = false;
    if (!vp.Is3D() && ImGui::IsWindowFocused() && dragMode == DragMode::NOT_DRAGGING)
    {
        const glm::vec3 pt = MapEditor::SnapToGrid(worldSpaceHover) + vp.Make3D(glm::vec2(0), 0.1);
        vpt = {
            .pos = pt,
            .color = Color(1, 0.7, 0.7, 1),
            .size = 10,
        };
        showPoint = true;
    }

    const ViewportRenderer::ViewportRenderSettings vps = {
        .brushFocusMode = false,
        .focusedBrushIndex = 0,
        .hoverType = ItemType::NONE,
        .hoverIndex = 0,
        .selectionType = ItemType::NONE,
        .selectionIndex = 0,
        .selectionVertexIndex = 0,
        .point = showPoint ? &vpt : nullptr,
        .newPrimitive = hasDrawnShape ? &newPrim : nullptr,
        .newActor = nullptr,
    };
    ViewportRenderer::RenderViewport(vp, vps);
}

std::vector<glm::vec2> AddPrimitiveTool::GetPoints() const
{
    std::vector<glm::vec2> points{};

    if (primitive == PrimitiveType::NGON)
    {
        points = BuildNgon(ngonSides, shapeStart, shapeEnd, ngonStartAngle);
    } else if (primitive == PrimitiveType::TRIANGLE)
    {
        points = BuildTriangle(shapeStart, shapeEnd);
    } else if (primitive == PrimitiveType::RECTANGLE)
    {
        points = BuildRect(shapeStart, shapeEnd);
    }
    return points;
}

void AddPrimitiveTool::RenderToolWindow()
{
    if (!ImGui::CollapsingHeader("Primitive Tool", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    ImGui::PushItemWidth(-1);
    MapEditor::MaterialSelectionTool(MapEditor::material);
    ImGui::Separator();

    ImGui::Text("Primitive Type");
    int type = static_cast<int>(primitive);
    if (ImGui::Combo("##primType", &type, PRIMITIVE_NAMES.data(), PRIMITIVE_NAMES.size()))
    {
        primitive = static_cast<PrimitiveType>(type);
    }

    if (primitive == PrimitiveType::NGON)
    {
        ImGui::Separator();
        ImGui::Text("Ngon Sides");
        ImGui::InputInt("##sides", &ngonSides);
        ngonSides = std::ranges::clamp(ngonSides, 3, 128);
        ImGui::Text("Ngon Angle");
        float deg = glm::degrees(ngonStartAngle);
        if (ImGui::InputFloat("##ngonAngle", &deg, 22.5f, 0, "%.2fdeg"))
        {
            deg = std::ranges::clamp(deg, -360.0f, 360.0f);
            ngonStartAngle = glm::radians(deg);
        }
        ImGui::Separator();
        const ImVec2 buttonSize = {ImGui::GetContentRegionAvail().x / 3 - 6, 0}; // TODO obtain 6 without using magic

        if (ImGui::Button("Hexagon", buttonSize))
        {
            ngonSides = 6;
        }
        ImGui::SameLine();
        if (ImGui::Button("Octagon", buttonSize))
        {
            ngonSides = 8;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cylinder", buttonSize))
        {
            ngonSides = 16;
        }
    }
}

std::vector<glm::vec2> AddPrimitiveTool::BuildNgon(const int n,
                                                   const glm::vec2 &p0,
                                                   const glm::vec2 &p1,
                                                   const float startAngleRadians)
{
    std::vector<glm::vec2> pts;
    if (n <= 0)
    {
        return pts;
    }

    const float left = std::min(p0.x, p1.x);
    const float right = std::max(p0.x, p1.x);
    const float top = std::min(p0.y, p1.y);
    const float bottom = std::max(p0.y, p1.y);

    const float cx = (left + right) * 0.5f;
    const float cy = (top + bottom) * 0.5f;

    const float boxWidth = right - left;
    const float boxHeight = bottom - top;

    const float rx = std::max(0.0f, boxWidth * 0.5f);
    const float ry = std::max(0.0f, boxHeight * 0.5f);

    pts.reserve(n);
    for (int i = 0; i < n; i++)
    {
        const float theta = startAngleRadians + 1 * (2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / n);
        const float x = cx + rx * std::cos(theta);
        const float y = cy + ry * std::sin(theta);
        pts.emplace_back(x, y);
    }
    return pts;
}

std::vector<glm::vec2> AddPrimitiveTool::BuildRect(const glm::vec2 &p0, const glm::vec2 &p1)
{
    std::vector<glm::vec2> pts;

    const float left = std::min(p0.x, p1.x);
    const float right = std::max(p0.x, p1.x);
    const float top = std::min(p0.y, p1.y);
    const float bottom = std::max(p0.y, p1.y);

    pts.emplace_back(left, top);
    pts.emplace_back(right, top);
    pts.emplace_back(right, bottom);
    pts.emplace_back(left, bottom);
    return pts;
}

std::vector<glm::vec2> AddPrimitiveTool::BuildTriangle(const glm::vec2 &p0, const glm::vec2 &p1)
{
    std::vector<glm::vec2> pts;

    const float left = std::min(p0.x, p1.x);
    const float right = std::max(p0.x, p1.x);
    const float top = std::min(p0.y, p1.y);
    const float bottom = std::max(p0.y, p1.y);

    const float cx = (left + right) * 0.5f;

    pts.emplace_back(cx, bottom);
    pts.emplace_back(right, top);
    pts.emplace_back(left, top);
    return pts;
}
