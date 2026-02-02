//
// Created by droc101 on 7/4/25.
//

#include "LodsTab.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <imgui.h>
#include <libassets/type/ModelLod.h>
#include <numeric>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <utility>
#include "../ModelRenderer.h"
#include "DialogFilters.h"

void LodsTab::Render(SDL_Window *window)
{
    ImGui::Begin("LODs",
                 nullptr,
                 ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PushItemWidth(-1);

    if (ImGui::Button("Add", ImVec2(70, 0)))
    {
        SDL_ShowOpenFileDialog(addLodCallback,
                               nullptr,
                               window,
                               DialogFilters::modelFilters.data(),
                               DialogFilters::modelFilters.size(),
                               nullptr,
                               false);
    }
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x - 70, 0));
    ImGui::SameLine();
    if (ImGui::Button("Validate", ImVec2(70, 0)))
    {
        if (!ModelRenderer::GetModel().ValidateLodDistances())
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING,
                                     "Warning",
                                     "LOD distances are invalid! Make sure that:\n"
                                     "- The first LOD (LOD 0) has a distance of 0\n"
                                     "- No two LODs have the same distance",
                                     window);
        } else
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Success", "LOD distances are valid!", window);
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
            SDL_ShowSaveFileDialog(saveLodCallback, &lod, window, DialogFilters::objFilters.data(), 1, nullptr);
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

void LodsTab::addLodCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    SDL_Event event;
    event.type = ModelRenderer::EVENT_RELOAD_MODEL;
    event.user.code = ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_LOD;
    event.user.data1 = new std::string(fileList[0]);
    if (!SDL_PushEvent(&event))
    {
        printf("Error: SDL_PushEvent(): %s\n", SDL_GetError());
    }
}

void LodsTab::saveLodCallback(void *userdata, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    static_cast<ModelLod *>(userdata)->Export(fileList[0]);
}
