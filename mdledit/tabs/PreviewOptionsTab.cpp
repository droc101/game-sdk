//
// Created by droc101 on 2/2/26.
//

#include "PreviewOptionsTab.h"
#include <imgui.h>
#include "../ModelEditor.h"

void PreviewOptionsTab::Render()
{
    ImGui::Begin("Preview Options", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::PushItemWidth(-1);
    ImGui::TextUnformatted("LOD");
    ImGui::SliderInt("##LOD",
                     &ModelEditor::modelViewer.lodIndex,
                     0,
                     static_cast<int>(ModelEditor::modelViewer.GetModel().GetLodCount() - 1));
    ImGui::TextUnformatted("Skin");
    ImGui::SliderInt("##Skin",
                     &ModelEditor::modelViewer.skinIndex,
                     0,
                     static_cast<int>(ModelEditor::modelViewer.GetModel().GetSkinCount() - 1));

    ImGui::SeparatorText("Display Options");
    if (ImGui::BeginTable("##optionsGrid", 3))
    {
        ImGui::TableNextColumn();
        ImGui::Checkbox("Wireframe", &ModelEditor::modelViewer.wireframe);
        ImGui::TableNextColumn();
        ImGui::Checkbox("Cull Backfaces", &ModelEditor::modelViewer.cullBackfaces);
        ImGui::TableNextColumn();
        ImGui::Checkbox("Show Unit Cube", &ModelEditor::modelViewer.showUnitCube);
        ImGui::TableNextColumn();
        ImGui::Checkbox("Show Bounding Box", &ModelEditor::modelViewer.showBoundingBox);
        ImGui::TableNextColumn();
        ImGui::Checkbox("Show Collision Model", &ModelEditor::modelViewer.showCollisionModel);
        ImGui::EndTable();
    }

    ImGui::Text("Background Color");
    ImGui::ColorEdit3("##bgColor", ModelEditor::modelViewer.backgroundColor.GetDataPointer());

    ImGui::SeparatorText("Display Mode");
    if (ImGui::RadioButton("Unshaded", ModelEditor::modelViewer.displayMode == ModelViewer::DisplayMode::COLORED))
    {
        ModelEditor::modelViewer.displayMode = ModelViewer::DisplayMode::COLORED;
    }
    if (ImGui::RadioButton("Shaded", ModelEditor::modelViewer.displayMode == ModelViewer::DisplayMode::COLORED_SHADED))
    {
        ModelEditor::modelViewer.displayMode = ModelViewer::DisplayMode::COLORED_SHADED;
    }
    if (ImGui::RadioButton("Textured Unshaded", ModelEditor::modelViewer.displayMode == ModelViewer::DisplayMode::TEXTURED))
    {
        ModelEditor::modelViewer.displayMode = ModelViewer::DisplayMode::TEXTURED;
    }
    if (ImGui::RadioButton("Textured Shaded",
                           ModelEditor::modelViewer.displayMode == ModelViewer::DisplayMode::TEXTURED_SHADED))
    {
        ModelEditor::modelViewer.displayMode = ModelViewer::DisplayMode::TEXTURED_SHADED;
    }
    if (ImGui::RadioButton("UV Debug", ModelEditor::modelViewer.displayMode == ModelViewer::DisplayMode::UV))
    {
        ModelEditor::modelViewer.displayMode = ModelViewer::DisplayMode::UV;
    }
    if (ImGui::RadioButton("Normal Debug", ModelEditor::modelViewer.displayMode == ModelViewer::DisplayMode::NORMAL))
    {
        ModelEditor::modelViewer.displayMode = ModelViewer::DisplayMode::NORMAL;
    }
    ImGui::End();
}
