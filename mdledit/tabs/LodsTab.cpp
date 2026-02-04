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
#include "../ModelRenderer.h"

// from mdledit.cpp
void importLod(const std::string &path);

void LodsTab::Render()
{
    ImGui::Begin("LODs", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::PushItemWidth(-1);
    if (ImGui::Button("Add", ImVec2(70, 0)))
    {
        SDKWindow::OpenFileDialog(importLod, DialogFilters::modelFilters);
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x - 70);
    ImGui::SliderInt("##LOD",
                     &ModelRenderer::lodIndex,
                     0,
                     static_cast<int>(ModelRenderer::GetModel().GetLodCount() - 1));
    ImGui::SameLine();
    // ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x - 70, 0));
    ImGui::SameLine();
    if (ImGui::Button("Validate", ImVec2(70, 0)))
    {
        if (!ModelRenderer::GetModel().ValidateLodDistances())
        {
            SDKWindow::WarningMessage("LOD distances are invalid! Make sure that:\n"
                                      "- The first LOD (LOD 0) has a distance of 0\n"
                                      "- No two LODs have the same distance");
        } else
        {
            SDKWindow::InfoMessage("LOD distances are valid", "Success");
        }
    }

    const float panelHeight = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("ScrollableRegion",
                      ImVec2(0, panelHeight),
                      ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
    for (size_t lodIndex = 0; lodIndex < ModelRenderer::GetModel().GetLodCount(); lodIndex++)
    {
        const std::string title = std::format("LOD {}", lodIndex);
        ImGui::SeparatorText(title.c_str());
        ModelLod &lod = ModelRenderer::GetModel().GetLod(lodIndex);
        const uint32_t tris = std::accumulate(lod.indexCounts.begin(), lod.indexCounts.end(), 0u) / 3u;
        ImGui::TextUnformatted(std::format("{} vertices, {} triangles", lod.vertices.size(), tris).c_str());
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
        ImGui::TextUnformatted("Distance");
        ImGui::PushItemWidth(-1);
        ImGui::InputFloat(std::format("##LOD_{}_Distance", lodIndex).c_str(), &lod.distance, 0.0f, 0.0f, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            ModelRenderer::GetModel().SortLODs();
        }
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
        const ImVec2 space = ImGui::GetContentRegionAvail();
        const float buttonWidth = (space.x / 3.0f) - 6.0f;
        if (ImGui::Button(std::format("Export##{}", lodIndex).c_str(), ImVec2(buttonWidth, 0)))
        {
            lodToExport = &lod;
            SDKWindow::SaveFileDialog(saveLodCallback, DialogFilters::objFilters);
        }
        ImGui::SameLine();
        if (ImGui::Button(std::format("Flip UVs##{}", lodIndex).c_str(), ImVec2(buttonWidth, 0)))
        {
            ModelAsset model = ModelRenderer::GetModel();
            model.GetLod(lodIndex).FlipVerticalUVs();
            ModelRenderer::LoadModel(std::move(model));
        }
        if (ModelRenderer::GetModel().GetLodCount() != 1)
        {
            ImGui::SameLine();
            if (ImGui::Button(std::format("Delete##{}", lodIndex).c_str(), ImVec2(buttonWidth, 0)))
            {
                ModelRenderer::GetModel().RemoveLod(lodIndex);
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
