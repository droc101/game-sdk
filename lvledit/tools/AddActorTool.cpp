//
// Created by droc101 on 10/20/25.
//

#include "AddActorTool.h"
#include <array>
#include <cstddef>
#include <imgui.h>
#include <libassets/type/Actor.h>
#include <libassets/type/Color.h>
#include <libassets/type/Sector.h>
#include <memory>
#include <ranges>
#include <unordered_set>
#include "../LevelEditor.h"
#include "../LevelRenderer.h"
#include "../Viewport.h"
#include "EditorTool.h"
#include "libassets/type/ActorDefinition.h"
#include "SelectTool.h"
#include "SharedMgr.h"

void AddActorTool::RenderViewport(Viewport &vp)
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

    if (isHovered)
    {
        if (!hasPlacedActor)
        {
            const glm::vec2 pt = LevelEditor::SnapToGrid(glm::vec2(worldSpaceHover.x, worldSpaceHover.z));
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                newActorPosition = LevelEditor::SnapToGrid(worldSpaceHover);
                hasPlacedActor = true;
            } else
            {
                LevelRenderer::RenderBillboardPoint(glm::vec3(pt.x, 0.1, pt.y), 10, Color(0.7, 1, 0.7, 1), matrix);
            }

            if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
            {
                LevelEditor::toolType = LevelEditor::EditorToolType::SELECT;
                LevelEditor::tool = std::unique_ptr<EditorTool>(new SelectTool());
                return;
            }
        } else
        {
            if (ImGui::Shortcut(ImGuiKey_Escape))
            {
                hasPlacedActor = false;
            } else if (ImGui::Shortcut(ImGuiKey_Enter) || ImGui::Shortcut(ImGuiKey_KeypadEnter))
            {
                Actor a{};
                a.className = newActorType;
                a.rotation = {0, 0, 0};
                a.position = {newActorPosition.x, newActorPosition.y, newActorPosition.z};

                ActorDefinition def = SharedMgr::actorDefinitions.at(newActorType);
                a.ApplyDefinition(def);

                LevelEditor::level.actors.push_back(a);
                hasPlacedActor = false;
            } else
            {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    const glm::vec3 snappedHover = LevelEditor::SnapToGrid(worldSpaceHover);
                    switch (vp.GetType())
                    {
                        case Viewport::ViewportType::TOP_DOWN_XZ:
                            newActorPosition.x = snappedHover.x;
                            newActorPosition.z = snappedHover.z;
                            break;
                        case Viewport::ViewportType::FRONT_XY:
                            newActorPosition.x = snappedHover.x;
                            newActorPosition.y = snappedHover.y;
                            break;
                        case Viewport::ViewportType::SIDE_YZ:
                            newActorPosition.y = snappedHover.y;
                            newActorPosition.z = snappedHover.z;
                            break;
                    }
                }
            }
        }
    }

    for (auto &sector: LevelEditor::level.sectors)
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

    for (Actor &a: LevelEditor::level.actors)
    {
        const glm::vec3 pos = glm::vec3(a.position.at(0), a.position.at(1), a.position.at(2));
        LevelRenderer::RenderBillboardPoint(pos, 10, Color(0.7, 1, 0.7, 1), matrix);
    }

    if (hasPlacedActor)
    {
        LevelRenderer::RenderBillboardPoint(newActorPosition, 10, Color(0, 1, 0, 1), matrix);
    }
}

void AddActorTool::RenderToolWindow()
{
    if (!ImGui::CollapsingHeader("Actor Tool", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }
    ImGui::PushItemWidth(-1);
    ImGui::Text("Class");
    if (ImGui::BeginCombo("##a", newActorType.c_str()))
    {
        for (const std::string &key: SharedMgr::actorDefinitions | std::views::keys)
        {
            if (SharedMgr::actorDefinitions.at(key).isVirtual)
            {
                continue;
            }
            if (newActorType == key)
            {
                ImGui::SetItemDefaultFocus();
            }
            if (ImGui::Selectable(key.c_str(), key == newActorType))
            {
                newActorType = key;
            }
        }
        ImGui::EndCombo();
    }
}
