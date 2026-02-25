//
// Created by droc101 on 9/19/25.
//

#include "AddPolygonTool.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <imgui.h>
#include <libassets/type/Color.h>
#include <libassets/type/Sector.h>
#include <libassets/type/WallMaterial.h>
#include <memory>
#include "../MapEditor.h"
#include "../MapRenderer.h"
#include "../Viewport.h"
#include "EditorTool.h"
#include "game_sdk/SDKWindow.h"
#include "SelectTool.h"

void AddPolygonTool::RenderToolWindow()
{
    if (!ImGui::CollapsingHeader("Polygon Tool", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    ImGui::PushItemWidth(-1);
    MapEditor::MaterialToolWindow(MapEditor::mat);
    ImGui::Separator();
    ImGui::Text("Ceiling Height");
    ImGui::InputFloat("##ceilHeight", &ceiling);
    ImGui::Text("Floor Height");
    ImGui::InputFloat("##floorHeight", &floor);
}

void AddPolygonTool::RenderViewport(Viewport &vp)
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

    if (isHovered && vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        if (!isDrawing)
        {
            const glm::vec2 pt = MapEditor::SnapToGrid(glm::vec2(worldSpaceHover.x, worldSpaceHover.z));
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                points = {pt};
                ceiling = 1;
                floor = -1;
                isDrawing = true;
            } else
            {
                MapRenderer::RenderBillboardPoint(glm::vec3(pt.x, 0.1, pt.y), 10, Color(1, 0.7, 0.7, 1), matrix);
            }

            if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
            {
                MapEditor::toolType = MapEditor::EditorToolType::SELECT;
                MapEditor::tool = std::unique_ptr<EditorTool>(new SelectTool());
                return;
            }
        } else
        {
            if (ImGui::Shortcut(ImGuiKey_Escape))
            {
                isDrawing = false;
            } else
            {
                const glm::vec2 firstPoint = points.at(0);
                const glm::vec3 worldSpaceFirstPoint = glm::vec3(firstPoint.x, worldSpaceHover.y, firstPoint.y);
                const glm::vec2 screenSpaceFirstPoint = vp.WorldToScreenPos(worldSpaceFirstPoint);
                if (distance(screenSpaceHover, screenSpaceFirstPoint) < 5 ||
                    MapEditor::SnapToGrid(worldSpaceHover) == worldSpaceFirstPoint)
                {
                    if (points.size() < 3)
                    {
                        if (ImGui::BeginTooltip())
                        {
                            ImGui::Text("Cannot create sector with less than 3 points");
                            ImGui::EndTooltip();
                        }
                    } else
                    {
                        if (ImGui::BeginTooltip())
                        {
                            ImGui::Text("Close Sector");
                            ImGui::EndTooltip();
                        }
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            isDrawing = false;
                            Sector s = Sector();
                            const WallMaterial mat = MapEditor::mat;
                            s.ceilingMaterial = mat;
                            s.floorMaterial = mat;
                            s.floorHeight = floor;
                            s.ceilingHeight = ceiling;
                            s.lightColor = Color(1, 1, 1, 1);
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
                        }
                    }
                } else
                {
                    const glm::vec2 worldSpacePoint = glm::vec2(worldSpaceHover.x, worldSpaceHover.z);
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
        }
    }

    for (auto &sector: MapEditor::map.sectors)
    {
        for (size_t vertexIndex = 0; vertexIndex < sector.points.size(); vertexIndex++)
        {
            const glm::vec2 &start2 = sector.points[vertexIndex];
            const glm::vec2 &end2 = sector.points[(vertexIndex + 1) % sector.points.size()];
            const glm::vec3 startCeiling = glm::vec3(start2.x, sector.ceilingHeight, start2.y);
            const glm::vec3 endCeiling = glm::vec3(end2.x, sector.ceilingHeight, end2.y);
            const glm::vec3 startFloor = glm::vec3(start2.x, sector.floorHeight, start2.y);
            const glm::vec3 endFloor = glm::vec3(end2.x, sector.floorHeight, end2.y);

            if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
            {
                MapRenderer::RenderBillboardPoint(startCeiling + glm::vec3(0, 0.1, 0),
                                                  10,
                                                  Color(1, 0.7, 0.7, 1),
                                                  matrix);
            }
            if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
            {
                MapRenderer::RenderLine(startFloor, endFloor, Color(0.7, .7, .7, 1), matrix, 4);
                MapRenderer::RenderLine(startCeiling, startFloor, Color(.6, .6, .6, 1), matrix, 4);
            }

            MapRenderer::RenderLine(startCeiling, endCeiling, Color(0.7, .7, .7, 1), matrix, 4);
        }
    }

    for (Actor &a: MapEditor::map.actors)
    {
        MapRenderer::RenderActor(a, matrix, vp);
    }

    if (isDrawing)
    {
        for (size_t vertexIndex = 0; vertexIndex < points.size(); vertexIndex++)
        {
            const glm::vec2 &start2 = points[vertexIndex];
            glm::vec2 end2 = points[(vertexIndex + 1) % points.size()];
            if (vertexIndex == points.size() - 1)
            {
                end2 = MapEditor::SnapToGrid(glm::vec2(worldSpaceHover.x, worldSpaceHover.z));
            }
            const glm::vec3 startCeiling = glm::vec3(start2.x, ceiling, start2.y);
            const glm::vec3 endCeiling = glm::vec3(end2.x, ceiling, end2.y);
            const glm::vec3 startFloor = glm::vec3(start2.x, floor, start2.y);
            const glm::vec3 endFloor = glm::vec3(end2.x, floor, end2.y);

            if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
            {
                MapRenderer::RenderBillboardPoint(startCeiling + glm::vec3(0, 0.1, 0), 10, Color(1, 0, 0, 1), matrix);
            }
            if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
            {
                MapRenderer::RenderLine(startFloor, endFloor, Color(1, 1, 1, 1), matrix, 4);
                MapRenderer::RenderLine(startCeiling, startFloor, Color(.6, .6, .6, 1), matrix, 4);
            }

            MapRenderer::RenderLine(startCeiling, endCeiling, Color(1, 1, 1, 1), matrix, 4);
        }
    }
}
