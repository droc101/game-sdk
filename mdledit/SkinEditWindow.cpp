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
#include "SharedMgr.h"

void SkinEditWindow::Render()
{
    ImGui::Begin("Skins",
                 nullptr,
                 ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 60 - 60 - 16);
    ImGui::TextUnformatted("Skin");
    ImGui::SliderInt("##Skin",
                     &ModelRenderer::skinIndex,
                     0,
                     static_cast<int>(ModelRenderer::GetModel().GetSkinCount() - 1));
    ImGui::SameLine();
    if (ImGui::Button("Add", ImVec2(60, 0)))
    {
        ModelRenderer::GetModel().AddSkin();
    }
    ImGui::BeginDisabled(ModelRenderer::GetModel().GetSkinCount() == 1);
    ImGui::SameLine();
    if (ImGui::Button("Remove", ImVec2(60, 0)))
    {
        ModelRenderer::GetModel().RemoveSkin(ModelRenderer::skinIndex);
        if (static_cast<size_t>(ModelRenderer::skinIndex) >= ModelRenderer::GetModel().GetSkinCount())
        {
            ModelRenderer::skinIndex = static_cast<int>(ModelRenderer::GetModel().GetSkinCount() - 1);
        }
    }
    ImGui::EndDisabled();

    const float panelHeight = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("ScrollableRegion",
                      ImVec2(0, panelHeight),
                      ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
    for (size_t i = 0; i < ModelRenderer::GetModel().GetMaterialsPerSkin(); i++)
    {
        ImGui::TextUnformatted(std::format("Slot {}", i).c_str());
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetCurrentContext()->Style.Colors[ImGuiCol_TextDisabled]);
        const uint32_t indexCount = ModelRenderer::GetModel().GetLod(ModelRenderer::lodIndex).indexCounts.at(i);
        ImGui::TextUnformatted(std::format("{} triangles", indexCount / 3).c_str());
        ImGui::PopStyleColor();

        size_t currentMaterialIndex = ModelRenderer::GetModel().GetSkin(ModelRenderer::skinIndex).at(i);
        Material &currentMaterial = ModelRenderer::GetModel().GetMaterial(currentMaterialIndex);
        ImTextureID currentTextureId;
        SharedMgr::textureCache->GetTextureID(currentMaterial.texture, currentTextureId);
        float *currentColor = currentMaterial.color.GetDataPointer();
        ImGui::Image(currentTextureId,
                     {18, 18},
                     {0, 0},
                     {1, 1},
                     {currentColor[0], currentColor[1], currentColor[2], currentColor[3]},
                     {0, 0, 0, 0});
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::BeginCombo(std::format("##materiallistbox_{}", i).c_str(),
                              std::format("Material {}: {}", currentMaterialIndex, currentMaterial.texture).c_str()))
        {
            for (size_t m = 0; m < ModelRenderer::GetModel().GetMaterialCount(); m++)
            {
                Material &mat = ModelRenderer::GetModel().GetMaterial(m);
                ImTextureID textureId;
                SharedMgr::textureCache->GetTextureID(mat.texture, textureId);
                const std::string title = std::format("##picker_{}_{}", m, i);
                bool selected = ImGui::Selectable(title.c_str(),
                                                  ModelRenderer::GetModel().GetSkin(ModelRenderer::skinIndex).at(i) ==
                                                          m,
                                                  ImGuiSelectableFlags_AllowOverlap |
                                                          ImGuiSelectableFlags_SpanAllColumns,
                                                  {0, 18});
                if (selected)
                {
                    ModelRenderer::GetModel().GetSkin(ModelRenderer::skinIndex).at(i) = m;
                }
                float *color = mat.color.GetDataPointer();
                ImGui::SameLine();
                ImGui::Image(textureId,
                             {18, 18},
                             {0, 0},
                             {1, 1},
                             {color[0], color[1], color[2], color[3]},
                             {0, 0, 0, 0});
                ImGui::SameLine();
                ImGui::Text("Material %zu: %s", m, mat.texture.c_str());
            }
            ImGui::EndCombo();
        }
    }
    ImGui::EndChild();

    ImGui::End();
}
