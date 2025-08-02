//
// Created by droc101 on 7/31/25.
//

#include "MaterialEditWindow.h"
#include <format>
#include "imgui.h"
#include "ModelRenderer.h"
#include "Options.h"
#include "TextureBrowserWindow.h"

void MaterialEditWindow::Show()
{
    visible = true;
}

void MaterialEditWindow::Hide()
{
    visible = false;
}

void MaterialEditWindow::Render()
{
    if (visible)
    {
        ImGui::SetNextWindowSize(ImVec2(300, -1));
        ImGui::Begin("Material Editor",
                     &visible,
                     ImGuiWindowFlags_NoCollapse);

        constexpr float panelHeight = 250.0f;
        ImGui::BeginChild("ScrollableRegion", ImVec2(0, panelHeight), ImGuiChildFlags_AutoResizeY);
        for (size_t m = 0; m < ModelRenderer::GetModel()->GetMaterialCount(); m++)
        {
            const std::string title = std::format("Material {}", m);
            if (ImGui::CollapsingHeader(title.c_str()))
            {
                Material &mat = ModelRenderer::GetModel()->GetMaterial(m);

                ImGui::PushItemWidth(-1);
                ImGui::Text("Texture");
                TextureBrowserWindow::InputTexture(std::format("##Texture{}", m).c_str(), mat.texture);
                ImGui::PushItemWidth(-1);

                ImGui::Text("Color");
                ImGui::ColorEdit4(std::format("##Color{}", m).c_str(), mat.color.GetData());

                ImGui::Text("Shader");
                constexpr std::array<const char *, 3> shaders = {"Sky", "Unshaded", "Shaded"};
                uint32_t sel = static_cast<uint32_t>(mat.shader);
                if (ImGui::BeginCombo(std::format("##Shader{}", m).c_str(), shaders.at(sel)))
                {
                    for (uint32_t i = 0; i < static_cast<uint32_t>(shaders.size()); i++)
                    {
                        const bool is_selected = sel == i;
                        if (ImGui::Selectable(shaders[i], is_selected))
                            sel = i;
                        mat.shader = static_cast<Material::MaterialShader>(sel);
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                if (ModelRenderer::GetModel()->GetMaterialCount() != 1)
                {
                    if (ImGui::Button(std::format("Delete##{}", m).c_str(), ImVec2(-1, 0)))
                    {
                        ModelAsset mdl = *ModelRenderer::GetModel();
                        mdl.RemoveMaterial(m);
                        ModelRenderer::LoadModel(mdl);
                    }
                }
            }
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        if (ImGui::Button("Add", ImVec2(60, 0)))
        {
            Material mat{};
            mat.texture = Options::defaultTexture;
            mat.color = Color({1.0f, 1.0f, 1.0f, 1.0f});
            mat.shader = Material::MaterialShader::SHADER_SHADED;
            ModelRenderer::GetModel()->AddMaterial(mat);
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