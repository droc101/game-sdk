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
        for (size_t m = 0; m < ModelRenderer::GetModel()->GetMaterialCount(); m++)
        {
            const std::string title = std::format("Material {}", m);
            if (ImGui::CollapsingHeader(title.c_str()))
            {
                ModelAsset::Material &mat = ModelRenderer::GetModel()->GetSkin(ModelRenderer::skin)[m];

                ImGui::PushItemWidth(-1);
                char *texBuf = mat.texture.data();
                ImGui::Text("Texture");
                ImGui::InputText(std::format("##Texture{}", m).c_str(), texBuf, 64);

                ImGui::Text("Color");
                ImGui::ColorEdit4(std::format("##Color{}", m).c_str(), mat.color.data());

                ImGui::Text("Shader");
                constexpr std::array<const char*, 3> shaders = {"Sky", "Unshaded", "Shaded"};
                int sel = static_cast<int>(mat.shader);
                if (ImGui::BeginCombo(std::format("##Shader{}", m).c_str(), shaders.at(sel)))
                {
                    for (int i = 0; i < static_cast<int>(shaders.size()); i++)
                    {
                        const bool is_selected = (sel == i);
                        if (ImGui::Selectable(shaders[i], is_selected))
                            sel = i;
                        mat.shader = static_cast<ModelAsset::ModelShader>(sel);
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
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

