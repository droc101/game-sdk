//
// Created by droc101 on 9/19/25.
//

#include "AddPolygonTool.h"
#include <algorithm>
#include <game_sdk/SDKWindow.h>
#include <imgui.h>
#include <libassets/type/Color.h>
#include <libassets/type/Sector.h>
#include <libassets/type/WallMaterial.h>
#include <memory>
#include "../MapEditor.h"
#include "../Viewport.h"
#include "../ViewportRenderer.h"
#include "EditorTool.h"
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

    ViewportRenderer::ViewportRenderNewPolygon sect = {
        .points = points,
        .floor = floor,
        .ceiling = ceiling,
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
        .newPrimitive = nullptr,
        .newActor = nullptr,
        .newPolygon = isDrawing ? &sect : nullptr,
    };
    ViewportRenderer::RenderViewport(vp, vps);
}
