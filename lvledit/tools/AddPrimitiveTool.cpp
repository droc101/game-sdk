//
// Created by droc101 on 9/21/25.
//

#include "AddPrimitiveTool.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <imgui.h>
#include <libassets/util/Sector.h>
#include <libassets/util/WallMaterial.h>
#include <memory>
#include <vector>
#include "../LevelEditor.h"
#include "../LevelRenderer.h"
#include "../Viewport.h"
#include "EditorTool.h"
#include "SelectTool.h"

void AddPrimitiveTool::RenderViewport(Viewport &vp)
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

        if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
        {
            if (!isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                shapeStart = LevelEditor::SnapToGrid(worldSpaceHover);
                shapeStart.y = ceiling;
                shapeEnd = LevelEditor::SnapToGrid(worldSpaceHover);
                shapeEnd.y = ceiling;
                isDragging = true;
                hasDrawnShape = true;
            } else if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                shapeEnd = LevelEditor::SnapToGrid(worldSpaceHover);
                shapeEnd.y = ceiling;
            } else
            {
                isDragging = false;
            }
        }
    }

    for (Sector &sector: LevelEditor::level.sectors)
    {
        for (size_t vertexIndex = 0; vertexIndex < sector.points.size(); vertexIndex++)
        {
            const std::array<float, 2> &start2 = sector.points[vertexIndex];
            const std::array<float, 2> &end2 = sector.points[(vertexIndex + 1) % sector.points.size()];
            const glm::vec3 startCeiling = glm::vec3(start2.at(0), sector.ceilingHeight, start2.at(1));
            const glm::vec3 endCeiling = glm::vec3(end2.at(0), sector.ceilingHeight, end2.at(1));
            const glm::vec3 startFloor = glm::vec3(start2.at(0), sector.floorHeight, start2.at(1));
            const glm::vec3 endFloor = glm::vec3(end2.at(0), sector.floorHeight, end2.at(1));

            if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
            {
                LevelRenderer::RenderBillboardPoint(startCeiling + glm::vec3(0, 0.1, 0),
                                                    10,
                                                    Color(1, 0.7, 0.7, 1),
                                                    matrix);
            }
            if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
            {
                LevelRenderer::RenderLine(startFloor, endFloor, Color(0.7, .7, .7, 1), matrix, 4);
                LevelRenderer::RenderLine(startCeiling, startFloor, Color(.6, .6, .6, 1), matrix, 4);
            }

            LevelRenderer::RenderLine(startCeiling, endCeiling, Color(0.7, .7, .7, 1), matrix, 4);
        }
    }

    if (hasDrawnShape)
    {
        const std::array<glm::vec3, 4> boxPoints = {
            shapeStart,
            glm::vec3(shapeStart.x, 0, shapeEnd.z),
            shapeEnd,
            glm::vec3(shapeEnd.x, 0, shapeStart.z),
        };
        for (size_t i = 0; i < boxPoints.size(); i++)
        {
            const size_t nextIndex = (i + 1) % boxPoints.size();
            const glm::vec3 startPointCeil = glm::vec3(boxPoints.at(i).x, ceiling, boxPoints.at(i).z);
            const glm::vec3 startPointFloor = glm::vec3(boxPoints.at(i).x, floor, boxPoints.at(i).z);
            const glm::vec3 endPointCeil = glm::vec3(boxPoints.at(nextIndex).x, ceiling, boxPoints.at(nextIndex).z);
            const glm::vec3 endPointFloor = glm::vec3(boxPoints.at(nextIndex).x, floor, boxPoints.at(nextIndex).z);
            LevelRenderer::RenderLine(startPointCeil, endPointCeil, Color(.6, 6, 0, 1), matrix, 2);
            if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
            {
                LevelRenderer::RenderLine(startPointFloor, endPointFloor, Color(.6, .6, 0, 1), matrix, 2);
                LevelRenderer::RenderLine(startPointCeil, startPointFloor, Color(.3, .3, 0, 1), matrix, 1);
            }
        }

        std::vector<glm::vec2> points{};

        if (primitive == PrimitiveType::NGON)
        {
            points = buildNgon(32, glm::vec2(shapeStart.x, shapeStart.z), glm::vec2(shapeEnd.x, shapeEnd.z));
            for (size_t i = 0; i < points.size(); i++)
            {
                const size_t nextIndex = (i + 1) % points.size();
                const glm::vec3 startPointCeil = glm::vec3(points.at(i).x, ceiling, points.at(i).y);
                const glm::vec3 startPointFloor = glm::vec3(points.at(i).x, floor, points.at(i).y);
                const glm::vec3 endPointCeil = glm::vec3(points.at(nextIndex).x, ceiling, points.at(nextIndex).y);
                const glm::vec3 endPointFloor = glm::vec3(points.at(nextIndex).x, floor, points.at(nextIndex).y);
                LevelRenderer::RenderLine(startPointCeil, endPointCeil, Color(1, 1, 0, 1), matrix, 4);
                if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
                {
                    LevelRenderer::RenderLine(startPointFloor, endPointFloor, Color(1, 1, 0, 1), matrix, 4);
                    LevelRenderer::RenderLine(startPointCeil, startPointFloor, Color(.6, .6, 0, 1), matrix, 2);
                }
            }


            points = buildNgon(ngonSides,
                               glm::vec2(shapeStart.x, shapeStart.z),
                               glm::vec2(shapeEnd.x, shapeEnd.z),
                               ngonStartAngle);
        } else if (primitive == PrimitiveType::TRIANGLE)
        {
            points = buildTri(glm::vec2(shapeStart.x, shapeStart.z), glm::vec2(shapeEnd.x, shapeEnd.z));
        } else if (primitive == PrimitiveType::RECTANGLE)
        {
            points = buildRect(glm::vec2(shapeStart.x, shapeStart.z), glm::vec2(shapeEnd.x, shapeEnd.z));
        }

        for (size_t i = 0; i < points.size(); i++)
        {
            const size_t nextIndex = (i + 1) % points.size();
            const glm::vec3 startPointCeil = glm::vec3(points.at(i).x, ceiling, points.at(i).y);
            const glm::vec3 startPointFloor = glm::vec3(points.at(i).x, floor, points.at(i).y);
            const glm::vec3 endPointCeil = glm::vec3(points.at(nextIndex).x, ceiling, points.at(nextIndex).y);
            const glm::vec3 endPointFloor = glm::vec3(points.at(nextIndex).x, floor, points.at(nextIndex).y);
            if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
            {
                LevelRenderer::RenderBillboardPoint(startPointCeil + glm::vec3(0, 0.1, 0),
                                                    10,
                                                    Color(1, 0, 0, 1),
                                                    matrix);
            }
            LevelRenderer::RenderLine(startPointCeil, endPointCeil, Color(1, 1, 1, 1), matrix, 4);
            if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
            {
                LevelRenderer::RenderLine(startPointFloor, endPointFloor, Color(1, 1, 1, 1), matrix, 4);
                LevelRenderer::RenderLine(startPointCeil, startPointFloor, Color(.6, .6, .6, 1), matrix, 2);
            }
        }


        if (ImGui::Shortcut(ImGuiKey_Enter) || ImGui::Shortcut(ImGuiKey_KeypadEnter))
        {
            Sector s = Sector();
            const WallMaterial mat = LevelEditor::mat;
            s.ceilingMaterial = mat;
            s.floorMaterial = mat;
            s.floorHeight = floor;
            s.ceilingHeight = ceiling;
            s.lightColor = Color(1, 1, 1, 1);
            for (const glm::vec2 &glmPoint: points)
            {
                const std::array<float, 2> point = {glmPoint.x, glmPoint.y};
                s.points.push_back(point);
                s.wallMaterials.push_back(mat);
            }
            LevelEditor::level.sectors.push_back(s);
            hasDrawnShape = false;
        } else if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
        {
            hasDrawnShape = false;
        }
    } else if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
    {
        LevelEditor::toolType = LevelEditor::EditorToolType::SELECT;
        LevelEditor::tool = std::unique_ptr<EditorTool>(new SelectTool());
    }
}

void AddPrimitiveTool::RenderToolWindow()
{
    if (!ImGui::CollapsingHeader("Primitive Tool", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    ImGui::PushItemWidth(-1);
    LevelEditor::MaterialToolWindow(LevelEditor::mat);
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
        const ImVec2 buttonSize = {(ImGui::GetContentRegionAvail().x / 3) - 6, 0}; // TODO obtain 6 without using magic

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

std::vector<glm::vec2> AddPrimitiveTool::buildNgon(const int n,
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
        const float theta = startAngleRadians + 1 * (2.0f * 3.14159265358979323846f * i / n);
        const float x = cx + rx * std::cos(theta);
        const float y = cy + ry * std::sin(theta);
        pts.emplace_back(x, y);
    }
    return pts;
}

std::vector<glm::vec2> AddPrimitiveTool::buildRect(const glm::vec2 &p0, const glm::vec2 &p1)
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

std::vector<glm::vec2> AddPrimitiveTool::buildTri(const glm::vec2 &p0, const glm::vec2 &p1)
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
