//
// Created by droc101 on 9/19/25.
//

#include "AddPolygonTool.h"
#include <array>
#include <cfloat>
#include <cstddef>
#include <imgui.h>
#include "../LevelEditor.h"
#include "../LevelRenderer.h"
#include "../Viewport.h"
#include "libassets/util/Color.h"
#include "libassets/util/Sector.h"

void AddPolygonTool::RenderToolWindow() {}

void AddPolygonTool::RenderViewport(Viewport &vp)
{
    LevelRenderer::RenderViewport(vp);

    ImGui::Text("WIP THIS TOOL DOES NOT DO ANYTHING AT THE MOMENT\nWIP THIS TOOL DOES NOT DO ANYTHING AT THE "
                "MOMENT\nWIP THIS TOOL DOES NOT DO ANYTHING AT THE MOMENT\nWIP THIS TOOL DOES NOT DO ANYTHING AT THE "
                "MOMENT\nWIP THIS TOOL DOES NOT DO ANYTHING AT THE MOMENT\n");

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

    for (size_t sectorIndex = 0; sectorIndex < LevelEditor::level.sectors.size(); sectorIndex++)
    {
        Sector &sector = LevelEditor::level.sectors.at(sectorIndex);
        for (size_t vertexIndex = 0; vertexIndex < sector.points.size(); vertexIndex++)
        {
            const std::array<float, 2> &start2 = sector.points[vertexIndex];
            const std::array<float, 2> &end2 = sector.points[(vertexIndex + 1) % sector.points.size()];
            const glm::vec3 startCeiling = glm::vec3(start2.at(0), sector.ceilingHeight, start2.at(1));
            const glm::vec3 endCeiling = glm::vec3(end2.at(0), sector.ceilingHeight, end2.at(1));
            const glm::vec3 startFloor = glm::vec3(start2.at(0), sector.floorHeight, start2.at(1));
            const glm::vec3 endFloor = glm::vec3(end2.at(0), sector.floorHeight, end2.at(1));

            if (vp.GetType() != Viewport::ViewportType::TOP_DOWN_XZ)
            {
                LevelRenderer::RenderLine(startFloor, endFloor, Color(0.7, .7, .7, 1), matrix, 4);
                LevelRenderer::RenderLine(startCeiling, startFloor, Color(.6, .6, .6, 1), matrix, 4);
            }

            LevelRenderer::RenderLine(startCeiling, endCeiling, Color(0.7, .7, .7, 1), matrix, 4);
        }
    }
}
