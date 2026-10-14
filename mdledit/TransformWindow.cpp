//
// Created by droc101 on 9/14/26.
//

#include "TransformWindow.h"
#include <cstddef>
#include <game_sdk/Window.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/ModelLod.h>
#include <utility>
#include "ModelEditor.h"

const Window::WindowProperties &TransformWindow::GetProperties() const
{
    return properties;
}

void TransformWindow::Render()
{
    ImGui::Text("Scale");
    ImGui::PushItemWidth(-1);
    ImGui::InputFloat3("##scale", glm::value_ptr(scale));
    ImGui::Separator();
    ImGui::Checkbox("Apply to LODs", &applyToLods);
    ImGui::SameLine();
    ImGui::Checkbox("Apply to collision", &applyToCollision);
    ImGui::Separator();
    if (ImGui::Button("OK"))
    {
        ModelAsset mdl = ModelEditor::modelViewer.GetModel();
        if (applyToCollision)
        {
            switch (mdl.GetCollisionModelType())
            {
                case ModelAsset::CollisionModelType::NONE:
                    break;
                case ModelAsset::CollisionModelType::STATIC_SINGLE_CONCAVE:
                    mdl.GetStaticCollisionMesh().Scale(scale);
                    break;
                case ModelAsset::CollisionModelType::DYNAMIC_MULTIPLE_CONVEX:
                    for (size_t i = 0; i < mdl.GetNumHulls(); i++)
                    {
                        mdl.GetHull(i).Scale(scale);
                    }
                    break;
            }
        }
        if (applyToLods)
        {
            for (size_t i = 0; i < mdl.GetLodCount(); i++)
            {
                mdl.GetLod(i).Scale(scale);
            }
        }

        if (updateBoundingBox)
        {
            mdl.GetBoundingBox() = BoundingBox(mdl.GetLod(0).vertices);
        }

        ModelEditor::modelViewer.SetModel(std::move(mdl));
        RequestClose();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        RequestClose();
    }
}
