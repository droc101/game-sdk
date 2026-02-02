//
// Created by droc101 on 2/2/26.
//

#include "PreviewOptionsWindow.h"
#include "imgui.h"
#include "ModelRenderer.h"

void PreviewOptionsWindow::Render()
{
    ImGui::Begin("Preview Options",
                 nullptr,
                 ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PushItemWidth(-1);
    ImGui::TextUnformatted("LOD");
    ImGui::SliderInt("##LOD",
                     &ModelRenderer::lodIndex,
                     0,
                     static_cast<int>(ModelRenderer::GetModel().GetLodCount() - 1));
    ImGui::TextUnformatted("Skin");
    ImGui::SliderInt("##Skin",
                     &ModelRenderer::skinIndex,
                     0,
                     static_cast<int>(ModelRenderer::GetModel().GetSkinCount() - 1));

    ImGui::SeparatorText("Display Options");
    ImGui::Checkbox("Wireframe", &ModelRenderer::wireframe);
    ImGui::Checkbox("Cull Backfaces", &ModelRenderer::cullBackfaces);
    ImGui::Checkbox("Show Unit Cube", &ModelRenderer::showUnitCube);
    ImGui::Checkbox("Show Bounding Box", &ModelRenderer::showBoundingBox);
    ImGui::Checkbox("Show Collision Model", &ModelRenderer::showCollisionModel);

    ImGui::SeparatorText("Display Mode");
    if (ImGui::RadioButton("Unshaded", ModelRenderer::displayMode == ModelRenderer::DisplayMode::COLORED))
    {
        ModelRenderer::displayMode = ModelRenderer::DisplayMode::COLORED;
    }
    if (ImGui::RadioButton("Shaded", ModelRenderer::displayMode == ModelRenderer::DisplayMode::COLORED_SHADED))
    {
        ModelRenderer::displayMode = ModelRenderer::DisplayMode::COLORED_SHADED;
    }
    if (ImGui::RadioButton("Textured Unshaded", ModelRenderer::displayMode == ModelRenderer::DisplayMode::TEXTURED))
    {
        ModelRenderer::displayMode = ModelRenderer::DisplayMode::TEXTURED;
    }
    if (ImGui::RadioButton("Textured Shaded",
                           ModelRenderer::displayMode == ModelRenderer::DisplayMode::TEXTURED_SHADED))
    {
        ModelRenderer::displayMode = ModelRenderer::DisplayMode::TEXTURED_SHADED;
    }
    if (ImGui::RadioButton("UV Debug", ModelRenderer::displayMode == ModelRenderer::DisplayMode::UV))
    {
        ModelRenderer::displayMode = ModelRenderer::DisplayMode::UV;
    }
    if (ImGui::RadioButton("Normal Debug", ModelRenderer::displayMode == ModelRenderer::DisplayMode::NORMAL))
    {
        ModelRenderer::displayMode = ModelRenderer::DisplayMode::NORMAL;
    }
    ImGui::End();
}
