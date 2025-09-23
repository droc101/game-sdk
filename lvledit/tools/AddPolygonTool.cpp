//
// Created by droc101 on 9/19/25.
//

#include "AddPolygonTool.h"
#include <array>
#include <cstddef>
#include <imgui.h>
#include "../LevelEditor.h"
#include "../LevelRenderer.h"
#include "../Viewport.h"
#include "libassets/util/Color.h"
#include "libassets/util/Sector.h"
#include "Options.h"
#include "libassets/util/WallMaterial.h"

void AddPolygonTool::RenderToolWindow()
{
    if (!ImGui::CollapsingHeader("Polygon Tool", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    ImGui::PushItemWidth(-1);
    ImGui::Text("No Options");
}

void AddPolygonTool::RenderViewport(Viewport &vp)
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

    if (isHovered && vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
    {
        if (!isDrawing && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            points = {LevelEditor::SnapToGrid(glm::vec2(worldSpaceHover.x, worldSpaceHover.z))};
            ceiling = 1;
            floor = -1;
            isDrawing = true;
        } else if (isDrawing)
        {
            const glm::vec2 firstPoint = points.at(0);
            const glm::vec3 worldSpaceFirstPoint = glm::vec3(firstPoint.x, worldSpaceHover.y, firstPoint.y);
            const glm::vec2 screenSpaceFirstPoint = vp.WorldToScreenPos(worldSpaceFirstPoint);
            if (distance(screenSpaceHover, screenSpaceFirstPoint) < 5 || LevelEditor::SnapToGrid(worldSpaceHover) == worldSpaceFirstPoint)
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
                        const WallMaterial mat = WallMaterial(Options::defaultTexture);
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
                    }
                }
            } else
            {
                const glm::vec2 worldSpacePoint = glm::vec2(worldSpaceHover.x, worldSpaceHover.z);
                // TODO don't allow placing the same point twice
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    points.push_back(LevelEditor::SnapToGrid(worldSpacePoint));
                }
            }
        }
    }

    for (auto & sector : LevelEditor::level.sectors)
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
                LevelRenderer::RenderBillboardPoint(startCeiling + glm::vec3(0, 0.1, 0), 10, Color(1, 0.7, 0.7, 1), matrix);
            }
            if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
            {
                LevelRenderer::RenderLine(startFloor, endFloor, Color(0.7, .7, .7, 1), matrix, 4);
                LevelRenderer::RenderLine(startCeiling, startFloor, Color(.6, .6, .6, 1), matrix, 4);
            }

            LevelRenderer::RenderLine(startCeiling, endCeiling, Color(0.7, .7, .7, 1), matrix, 4);
        }
    }

    if (isDrawing)
    {
        for (size_t vertexIndex = 0; vertexIndex < points.size(); vertexIndex++)
        {
            const glm::vec2 &start2 = points[vertexIndex];
            glm::vec2 end2 = points[(vertexIndex + 1) % points.size()];
            if (vertexIndex == points.size() - 1)
            {
                end2 = LevelEditor::SnapToGrid(glm::vec2(worldSpaceHover.x, worldSpaceHover.z));
            }
            const glm::vec3 startCeiling = glm::vec3(start2.x, ceiling, start2.y);
            const glm::vec3 endCeiling = glm::vec3(end2.x, ceiling, end2.y);
            const glm::vec3 startFloor = glm::vec3(start2.x, floor, start2.y);
            const glm::vec3 endFloor = glm::vec3(end2.x, floor, end2.y);

            if (vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ)
            {
                LevelRenderer::RenderBillboardPoint(startCeiling + glm::vec3(0, 0.1, 0), 10, Color(1, 0, 0, 1), matrix);
            }
            if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
            {
                LevelRenderer::RenderLine(startFloor, endFloor, Color(1, 1, 1, 1), matrix, 4);
                LevelRenderer::RenderLine(startCeiling, startFloor, Color(.6, .6, .6, 1), matrix, 4);
            }

            LevelRenderer::RenderLine(startCeiling, endCeiling, Color(1, 1, 1, 1), matrix, 4);
        }
    }
}
