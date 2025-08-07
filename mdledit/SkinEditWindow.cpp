//
// Created by droc101 on 7/1/25.
//

#include "SkinEditWindow.h"
#include <cstddef>
#include <cstdint>
#include <format>
#include <imgui.h>
#include <imgui_internal.h>
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
        ImGui::Begin("Skin Editor", &visible, ImGuiWindowFlags_NoCollapse);
        ImGui::PushItemWidth(-1);
        ImGui::TextUnformatted("Skin");
        ImGui::SliderInt("##Skin",
                         &ModelRenderer::skinIndex,
                         0,
                         static_cast<int>(ModelRenderer::GetModel().GetSkinCount() - 1));
        ImGui::Dummy(ImVec2(0.0f, 16.0f));

        constexpr float panelHeight = 250.0f;
        ImGui::BeginChild("ScrollableRegion", ImVec2(0, panelHeight), ImGuiChildFlags_AutoResizeY);
        for (size_t i = 0; i < ModelRenderer::GetModel().GetMaterialsPerSkin(); i++)
        {
            int index = static_cast<int>(ModelRenderer::GetModel().GetSkin(ModelRenderer::skinIndex).at(i));

            ImGui::TextUnformatted(std::format("Slot {}", i).c_str());
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetCurrentContext()->Style.Colors[ImGuiCol_TextDisabled]);
            const uint32_t indexCount = ModelRenderer::GetModel().GetLod(ModelRenderer::lodIndex).indexCounts.at(i);
            ImGui::TextUnformatted(std::format("{} triangles", indexCount / 3).c_str());
            ImGui::PopStyleColor();
            ImGui::PushItemWidth(-1);
            if (ImGui::SliderInt(std::format("##index_{}", i).c_str(),
                                 &index,
                                 0,
                                 static_cast<int>(ModelRenderer::GetModel().GetMaterialCount() - 1)))
            {
                ModelRenderer::GetModel().GetSkin(ModelRenderer::skinIndex).at(i) = index;
            }
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        if (ImGui::Button("Add", ImVec2(60, 0)))
        {
            ModelRenderer::GetModel().AddSkin();
        }
        if (ModelRenderer::GetModel().GetSkinCount() > 1)
        {
            ImGui::SameLine();
            if (ImGui::Button("Remove", ImVec2(60, 0)))
            {
                ModelRenderer::GetModel().RemoveSkin(ModelRenderer::skinIndex);
                if (static_cast<size_t>(ModelRenderer::skinIndex) >= ModelRenderer::GetModel().GetSkinCount())
                {
                    ModelRenderer::skinIndex = static_cast<int>(ModelRenderer::GetModel().GetSkinCount() - 1);
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
