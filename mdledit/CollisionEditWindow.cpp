//
// Created by droc101 on 8/20/25.
//

#include "CollisionEditWindow.h"
#include <SDL3/SDL_video.h>
#include <imgui.h>
#include "ModelRenderer.h"

void CollisionEditWindow::Show()
{
    visible = true;
}

void CollisionEditWindow::Hide()
{
    visible = false;
}

void CollisionEditWindow::Render(SDL_Window *window)
{
    if (visible)
    {
        ModelAsset &model = ModelRenderer::GetModel();
        ImGui::SetNextWindowSize(ImVec2(300, -1));
        ImGui::Begin("Collision Editor", &visible, ImGuiWindowFlags_NoCollapse);
        ImGui::Text("Bounding Box");
        ImGui::InputFloat3("Origin", model.GetBoundingBox().origin.data());
        ImGui::InputFloat3("Extents", model.GetBoundingBox().extents.data());
        if (ImGui::Button("Autocalculate"))
        {
            model.GetBoundingBox() = BoundingBox(model.GetLod(0).vertices);
        }
        ImGui::PushItemWidth(-1);
        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x - 60, 0));
        ImGui::SameLine();
        if (ImGui::Button("OK", ImVec2(60, 0)))
        {
            visible = false;
        }

        ImGui::End();
    }
}
