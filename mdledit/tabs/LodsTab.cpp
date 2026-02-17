//
// Created by droc101 on 7/4/25.
//

#include "LodsTab.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SDKWindow.h>
#include <imgui.h>
#include <libassets/type/ModelLod.h>
#include <numeric>
#include <SDL3/SDL_events.h>
#include <string>
#include <utility>
#include "../ModelEditor.h"

void LodsTab::Render()
{
    ImGui::Begin("LODs", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::PushItemWidth(-1);
    if (ImGui::Button("Add", ImVec2(70, 0)))
    {
        SDKWindow::Get().OpenFileDialog(ModelEditor::ImportLod, DialogFilters::modelFilters);
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x - 70);
    ImGui::SliderInt("##LOD",
                     &ModelEditor::modelViewer.lodIndex,
                     0,
                     static_cast<int>(ModelEditor::modelViewer.GetModel().GetLodCount() - 1));
    ImGui::SameLine();
    // ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x - 70, 0));
    ImGui::SameLine();
    if (ImGui::Button("Validate", ImVec2(70, 0)))
    {
        if (!ModelEditor::modelViewer.GetModel().ValidateLodDistances())
        {
            SDKWindow::Get().WarningMessage("LOD distances are invalid! Make sure that:\n"
                                            "- The first LOD (LOD 0) has a distance of 0\n"
                                            "- No two LODs have the same distance");
        } else
        {
            SDKWindow::Get().InfoMessage("LOD distances are valid", "Success");
        }
    }

    const float panelHeight = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("ScrollableRegion",
                      ImVec2(0, panelHeight),
                      ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
    for (size_t lodIndex = 0; lodIndex < ModelEditor::modelViewer.GetModel().GetLodCount(); lodIndex++)
    {
        const std::string title = std::format("LOD {}", lodIndex);
        ImGui::SeparatorText(title.c_str());
        ModelLod &lod = ModelEditor::modelViewer.GetModel().GetLod(lodIndex);
        const uint32_t tris = std::accumulate(lod.indexCounts.begin(), lod.indexCounts.end(), 0u) / 3u;
        ImGui::TextUnformatted(std::format("{} vertices, {} triangles", lod.vertices.size(), tris).c_str());
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
        ImGui::TextUnformatted("Distance");
        ImGui::PushItemWidth(-1);
        ImGui::InputFloat(std::format("##LOD_{}_Distance", lodIndex).c_str(), &lod.distance, 0.0f, 0.0f, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            ModelEditor::modelViewer.GetModel().SortLODs();
            ModelEditor::modelViewer.ReloadModel();
        }
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
        const ImVec2 space = ImGui::GetContentRegionAvail();
        const float buttonWidth = (space.x / 3.0f) - 6.0f;
        if (ImGui::Button(std::format("Export##{}", lodIndex).c_str(), ImVec2(buttonWidth, 0)))
        {
            lodToExport = &lod;
            SDKWindow::Get().SaveFileDialog(saveLodCallback, DialogFilters::objFilters);
        }
        ImGui::SameLine();
        if (ImGui::Button(std::format("Flip UVs##{}", lodIndex).c_str(), ImVec2(buttonWidth, 0)))
        {
            ModelAsset model = ModelEditor::modelViewer.GetModel();
            model.GetLod(lodIndex).FlipVerticalUVs();
            ModelEditor::modelViewer.SetModel(std::move(model));
        }
        if (ModelEditor::modelViewer.GetModel().GetLodCount() != 1)
        {
            ImGui::SameLine();
            if (ImGui::Button(std::format("Delete##{}", lodIndex).c_str(), ImVec2(buttonWidth, 0)))
            {
                ModelEditor::modelViewer.GetModel().RemoveLod(lodIndex);
                ModelEditor::modelViewer.ReloadModel();
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void LodsTab::saveLodCallback(const std::string &path)
{
    lodToExport->Export(path.c_str());
}
