//
// Created by droc101 on 7/1/25.
//

#include "SkinEditWindow.h"
#include <format>
#include "imgui.h"
#include "ModelRenderer.h"

void SkinEditWindow::Show()
{
    visible = true;
}

void SkinEditWindow::Hide()
{
    visible = false;
}

void SkinEditWindow::Render()
{
    if (visible)
    {
        ImGui::SetNextWindowSize(ImVec2(300, -1));
        ImGui::Begin("Skin Editor",
                     &visible,
                     ImGuiWindowFlags_NoCollapse);
        ImGui::PushItemWidth(-1);
        ImGui::Text("Skin");
        ImGui::SliderInt("##Skin",
                         &ModelRenderer::skin,
                         0,
                         static_cast<int>(ModelRenderer::GetModel()->GetSkinCount() - 1));
        ImGui::Dummy(ImVec2(0.0f, 16.0f));

        constexpr float panelHeight = 250.0f;
        ImGui::BeginChild("ScrollableRegion", ImVec2(0, panelHeight), ImGuiChildFlags_AutoResizeY);
        for (size_t m = 0; m < ModelRenderer::GetModel()->GetMaterialsPerSkin(); m++)
        {
            int index = static_cast<int>(ModelRenderer::GetModel()->GetSkin(ModelRenderer::skin)[m]);

            ImGui::Text(std::format("Slot {}", m).c_str());
            ImGui::SameLine();
            ImGui::TextDisabled(std::format("{} triangles", ModelRenderer::GetModel()->GetLod(ModelRenderer::lod).indexCounts[m] / 3).c_str());
            ImGui::PushItemWidth(-1);
            if (ImGui::SliderInt(std::format("##index_{}", m).c_str(), &index, 0, static_cast<int>(ModelRenderer::GetModel()->GetMaterialCount() - 1)))
            {
                ModelRenderer::GetModel()->GetSkin(ModelRenderer::skin)[m] = index;
            }
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        if (ImGui::Button("Add", ImVec2(60, 0)))
        {
            ModelRenderer::GetModel()->AddSkin();
        }
        if (ModelRenderer::GetModel()->GetSkinCount() > 1)
        {
            ImGui::SameLine();
            if (ImGui::Button("Remove", ImVec2(60, 0)))
            {
                ModelRenderer::GetModel()->RemoveSkin(ModelRenderer::skin);
                if (static_cast<size_t>(ModelRenderer::skin) >= ModelRenderer::GetModel()->GetSkinCount())
                {
                    ModelRenderer::skin = static_cast<int>(ModelRenderer::GetModel()->GetSkinCount() - 1);
                }
            }
        }
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x - 60, 0));
        ImGui::SameLine();
        if (ImGui::Button("OK", ImVec2(60, 0)))
        {
            visible = false;
        }

        ImGui::End();
    }
}
