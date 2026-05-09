//
// Created by droc101 on 10/20/25.
//

#include "AddActorTool.h"
#include <imgui.h>
#include <libassets/type/Actor.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/Color.h>
#include <memory>
#include "../MapEditor.h"
#include "../Viewport.h"
#include "../ViewportRenderer.h"
#include "EditorTool.h"
#include "SelectTool.h"

void AddActorTool::RenderViewport(Viewport &vp)
{
    bool isHovered = false;
    glm::vec3 worldSpaceHover{};

    if (ImGui::IsWindowFocused())
    {
        isHovered = ImGui::IsWindowHovered();
        if (isHovered)
        {
            worldSpaceHover = vp.GetWorldSpaceMousePos();
        }
    }

    if (isHovered)
    {
        if (!hasPlacedActor)
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                newActorPosition = MapEditor::SnapToGrid(worldSpaceHover);
                hasPlacedActor = true;
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
                hasPlacedActor = false;
            } else if (ImGui::Shortcut(ImGuiKey_Enter) || ImGui::Shortcut(ImGuiKey_KeypadEnter))
            {
                Actor a{};
                a.className = newActorType;
                a.rotation = {0, 0, 0};
                a.position = {newActorPosition.x, newActorPosition.y, newActorPosition.z};

                const ActorDefinition &def = MapEditor::adm.GetActorDefinition(newActorType);
                a.ApplyDefinition(def, true);

                MapEditor::map.actors.push_back(a);
                hasPlacedActor = false;
            } else
            {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    const glm::vec3 snappedHover = MapEditor::SnapToGrid(worldSpaceHover);
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

    ViewportRenderer::ViewportRenderNewActor vpa = {
        .className = newActorType,
        .position = newActorPosition,
        .rotation = {0, 0, 0},
    };
    const glm::vec2 pt = MapEditor::SnapToGrid(glm::vec2(worldSpaceHover.x, worldSpaceHover.z));
    ViewportRenderer::ViewportRenderPoint vpt = {
        .pos = glm::vec3(pt.x, 0.1, pt.y),
        .color = Color(0.7, 1, 0.7, 1),
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
        .point = !hasPlacedActor && vp.GetType() == Viewport::ViewportType::TOP_DOWN_XZ ? &vpt : nullptr,
        .newPrimitive = nullptr,
        .newActor = hasPlacedActor ? &vpa : nullptr,
    };
    ViewportRenderer::RenderViewport(vp, vps);
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
        for (const std::string &key: MapEditor::adm.GetActorClasses())
        {
            if (MapEditor::adm.GetActorDefinition(key).isVirtual)
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
