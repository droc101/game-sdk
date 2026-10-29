//
// Created by droc101 on 10/29/26.
//

#include "MapeditOptionsWindow.h"
#include <game_sdk/Window.h>
#include <imgui.h>
#include "MapEditor.h"

const Window::WindowProperties &MapeditOptionsWindow::GetProperties() const
{
    return properties;
}

void MapeditOptionsWindow::Render()
{
    if (ImGui::BeginTabBar("##optionsTabs"))
    {
        if (ImGui::BeginTabItem("Appearance"))
        {
            const float tabHeight = ImGui::GetContentRegionAvail().y - 32;
            ImGui::BeginChild("##tabBody", {-1, tabHeight});

            ImGui::SeparatorText("General");
            ImGui::Checkbox("Show viewport information", &MapEditor::options.drawViewportInfo);
            ImGui::Checkbox("Show gizmos", &MapEditor::options.drawGizmos);
            ImGui::Checkbox("Enable culling", &MapEditor::options.culling);
            ImGui::Checkbox("Show sidebar", &MapEditor::options.showSidebar);

            ImGui::SeparatorText("2D viewports");
            ImGui::Checkbox("Show 2D viewports", &MapEditor::options.show2dViewports);
            ImGui::Checkbox("Show grid", &MapEditor::options.drawGrid);
            ImGui::Checkbox("Show axes", &MapEditor::options.drawAxisHelper);
            ImGui::Checkbox("Show world border", &MapEditor::options.drawWorldBorder);
            ImGui::Checkbox("Show 3D models in 2D viewports", &MapEditor::options.drawModels);

            ImGui::SeparatorText("3D viewport");
            ImGui::Checkbox("Show internal buffers", &MapEditor::options.showInternalBuffers);
            ImGui::Checkbox("Show invisible faces", &MapEditor::options.showInvisibleFaces);

            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Behavior"))
        {
            const float tabHeight = ImGui::GetContentRegionAvail().y - 32;
            ImGui::BeginChild("##tabBody", {-1, tabHeight});

            ImGui::TextUnformatted("Default Material");
            (void)MapEditor::MaterialSelectionTool(MapEditor::options.defaultMaterial);

            ImGui::Dummy({0, 16});

            ImGui::Checkbox("Snap to Grid", &MapEditor::options.snapToGrid);

            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::Dummy(ImVec2(0, 1));

    const float sizeX = ImGui::GetContentRegionAvail().x;

    ImGui::Dummy(ImVec2(sizeX - 60 - ImGui::GetStyle().WindowPadding.x, 0));
    ImGui::SameLine();
    if (ImGui::Button("OK", ImVec2(60, 0)))
    {
        MapEditor::options.Save();
        RequestClose();
    }
}
