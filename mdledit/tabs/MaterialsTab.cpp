//
// Created by droc101 on 7/31/25.
//

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <imgui.h>
#include <libassets/type/Material.h>
#include <string>
#include "../ModelRenderer.h"
#include "MaterialsTab.h"
#include "Options.h"
#include "SharedMgr.h"
#include "TextureBrowserWindow.h"

void MaterialsTab::Render()
{
    ImGui::Begin("Materials",
                 nullptr,
                 ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse);

    if (ImGui::Button("Add", ImVec2(60, 0)))
    {
        Material mat{};
        mat.texture = Options::defaultTexture;
        mat.color = Color(1.0f, 1.0f, 1.0f, 1.0f);
        mat.shader = Material::MaterialShader::SHADER_SHADED;
        ModelRenderer::GetModel().AddMaterial(mat);
    }

    if (selectedIndex >= ModelRenderer::GetModel().GetMaterialCount())
    {
        selectedIndex = ModelRenderer::GetModel().GetMaterialCount() - 1;
    }

    ImGui::PushItemWidth(-1);
    if (ImGui::BeginListBox("##materialPicker", {0, 150}))
    {
        for (size_t m = 0; m < ModelRenderer::GetModel().GetMaterialCount(); m++)
        {
            Material &mat = ModelRenderer::GetModel().GetMaterial(m);
            ImTextureID textureId;
            SharedMgr::textureCache->GetTextureID(mat.texture, textureId);
            const std::string title = std::format("##picker_{}", m);
            bool selected = ImGui::Selectable(title.c_str(),
                                              selectedIndex == m,
                                              ImGuiSelectableFlags_AllowOverlap | ImGuiSelectableFlags_SpanAllColumns,
                                              {0, 18});
            if (selected)
            {
                selectedIndex = m;
            }
            float *color = mat.color.GetDataPointer();
            ImGui::SameLine();
            ImGui::Image(textureId, {18, 18}, {0, 0}, {1, 1}, {color[0], color[1], color[2], color[3]}, {0, 0, 0, 0});
            ImGui::SameLine();
            ImGui::Text("Material %zu: %s", m, mat.texture.c_str());
        }
        ImGui::EndListBox();
    }

    Material &mat = ModelRenderer::GetModel().GetMaterial(selectedIndex);


    ImGui::PushItemWidth(-1);
    ImTextureID tid{};
    const Error::ErrorCode e = SharedMgr::textureCache->GetTextureID(mat.texture, tid);
    ImVec2 sz = ImGui::GetContentRegionAvail();
    if (e == Error::ErrorCode::OK)
    {
        constexpr int imagePanelHeight = 128;
        ImVec2 imageSize{};
        SharedMgr::textureCache->GetTextureSize(mat.texture, imageSize);
        const glm::vec2 scales = {(sz.x - 16) / imageSize.x, imagePanelHeight / imageSize.y};
        const float scale = std::ranges::min(scales.x, scales.y);
        float *color = mat.color.GetDataPointer();

        imageSize = {imageSize.x * scale, imageSize.y * scale};
        if (ImGui::BeginChild("##imageBox",
                              {sz.x, imagePanelHeight + 16},
                              ImGuiChildFlags_Borders,
                              ImGuiWindowFlags_NoResize))
        {
            sz = ImGui::GetContentRegionAvail();
            ImVec2 pos = ImGui::GetCursorPos();
            pos.x += (sz.x - imageSize.x) * 0.5f;
            pos.y += (sz.y - imageSize.y) * 0.5f;

            ImGui::SetCursorPos(pos);
            ImGui::Image(tid, imageSize, {0, 0}, {1, 1}, {color[0], color[1], color[2], color[3]}, {0, 0, 0, 0});
        }
        ImGui::EndChild();
    }

    ImGui::PushItemWidth(-1);
    ImGui::TextUnformatted("Texture");
    TextureBrowserWindow::InputTexture("##texture", mat.texture);
    ImGui::PushItemWidth(-1);

    ImGui::TextUnformatted("Color");
    ImGui::ColorEdit4("##color", mat.color.GetDataPointer());

    ImGui::TextUnformatted("Shader");
    constexpr std::array<const char *, 3> shaders = {"Sky", "Unshaded", "Shaded"};
    size_t sel = static_cast<uint32_t>(mat.shader);
    if (ImGui::BeginCombo("##shader", shaders.at(sel)))
    {
        for (size_t i = 0; i < shaders.size(); i++)
        {
            const bool is_selected = sel == i;
            if (ImGui::Selectable(shaders.at(i), is_selected))
            {
                sel = i;
            }
            mat.shader = static_cast<Material::MaterialShader>(sel);
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::Dummy({0, 8});
    if (ModelRenderer::GetModel().GetMaterialCount() != 1)
    {
        if (ImGui::Button("Delete Material", ImVec2(-1, 0)))
        {
            ModelRenderer::GetModel().RemoveMaterial(selectedIndex);
        }
    }

    ImGui::End();
}
