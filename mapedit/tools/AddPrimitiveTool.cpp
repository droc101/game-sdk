//
// Created by droc101 on 9/21/25.
//

#include "AddPrimitiveTool.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <game_sdk/SDKWindow.h>
#include <imgui.h>
#include <libassets/type/Sector.h>
#include <libassets/type/WallMaterial.h>
#include <memory>
#include <numbers>
#include <vector>
#include "../MapEditor.h"
#include "../Viewport.h"
#include "../ViewportRenderer.h"
#include "EditorTool.h"
#include "SelectTool.h"

void AddPrimitiveTool::RenderViewport(Viewport &vp)
{
    glm::vec3 worldSpaceHover{};

    if (ImGui::IsWindowFocused())
    {
        if (ImGui::IsWindowHovered())
        {
            worldSpaceHover = vp.GetWorldSpaceMousePos();
        }

        if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
        {
            if (!isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                shapeStart = MapEditor::SnapToGrid(worldSpaceHover);
                shapeStart.y = ceiling;
                shapeEnd = MapEditor::SnapToGrid(worldSpaceHover);
                shapeEnd.y = ceiling;
                isDragging = true;
                hasDrawnShape = true;
            } else if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                shapeEnd = MapEditor::SnapToGrid(worldSpaceHover);
                shapeEnd.y = ceiling;
            } else if (hasDrawnShape && isDragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                if (shapeStart == shapeEnd)
                {
                    hasDrawnShape = false;
                    isDragging = false;
                } else
                {
                    Sector s = Sector();
                    const WallMaterial mat = MapEditor::mat;
                    s.ceilingMaterial = mat;
                    s.floorMaterial = mat;
                    s.floorHeight = floor;
                    s.ceilingHeight = ceiling;
                    const std::vector<glm::vec2> points = GetPoints();
                    for (const glm::vec2 &glmPoint: points)
                    {
                        s.points.push_back(glmPoint);
                        s.wallMaterials.push_back(mat);
                    }
                    if (!s.IsValid())
                    {
                        SDKWindow::Get().ErrorMessage("Sector has invalid shape and will not be added");
                    } else
                    {
                        MapEditor::map.sectors.push_back(s);
                    }
                    hasDrawnShape = false;
                    isDragging = false;
                }
            } else
            {
                isDragging = false;
            }
        }
    }

    if (hasDrawnShape)
    {
        if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
        {
            hasDrawnShape = false;
        }
    } else if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
    {
        MapEditor::toolType = MapEditor::EditorToolType::SELECT;
        MapEditor::tool = std::unique_ptr<EditorTool>(new SelectTool());
    }

    ViewportRenderer::ViewportRenderNewPrimitive sect = {
        .points = GetPoints(),
        .floor = floor,
        .ceiling = ceiling,
        .aabbStart = shapeStart,
        .aabbEnd = shapeEnd,
    };
    const glm::vec2 pt = MapEditor::SnapToGrid(glm::vec2(worldSpaceHover.x, worldSpaceHover.z));
    ViewportRenderer::ViewportRenderPoint vpt = {
        .pos = glm::vec3(pt.x, 0.1, pt.y),
        .color = Color(1, 0.7, 0.7, 1),
        .size = 10,
    };
    const ViewportRenderer::ViewportRenderSettings vps = {
        .sectorFocusMode = false,
        .focusedSectorIndex = 0,
        .hoverType = ItemType::NONE,
        .hoverIndex = 0,
        .selectionType = ItemType::NONE,
        .selectionIndex = 0,
        .selectionVertexIndex = 0,
        .point = vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ ? &vpt : nullptr,
        .newPrimitive = hasDrawnShape ? &sect : nullptr,
        .newActor = nullptr,
    };
    ViewportRenderer::RenderViewport(vp, vps);
}

std::vector<glm::vec2> AddPrimitiveTool::GetPoints() const
{
    std::vector<glm::vec2> points{};

    if (primitive == PrimitiveType::NGON)
    {
        points = BuildNgon(ngonSides,
                           glm::vec2(shapeStart.x, shapeStart.z),
                           glm::vec2(shapeEnd.x, shapeEnd.z),
                           ngonStartAngle);
    } else if (primitive == PrimitiveType::TRIANGLE)
    {
        points = BuildTriangle(glm::vec2(shapeStart.x, shapeStart.z), glm::vec2(shapeEnd.x, shapeEnd.z));
    } else if (primitive == PrimitiveType::RECTANGLE)
    {
        points = BuildRect(glm::vec2(shapeStart.x, shapeStart.z), glm::vec2(shapeEnd.x, shapeEnd.z));
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
    MapEditor::MaterialToolWindow(MapEditor::mat);
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

    ImGui::Separator();
    ImGui::Text("Ceiling Height");
    ImGui::InputFloat("##ceilHeight", &ceiling);
    ImGui::Text("Floor Height");
    ImGui::InputFloat("##floorHeight", &floor);
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
