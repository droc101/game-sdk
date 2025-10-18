//
// Created by droc101 on 7/4/25.
//

#include "LodEditWindow.h"
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
#include "DialogFilters.h"
#include "ModelRenderer.h"

void LodEditWindow::Show()
{
    visible = true;
}

void LodEditWindow::Hide()
{
    visible = false;
}

void LodEditWindow::Render(SDL_Window *window)
{
    if (visible)
    {
        ImGui::SetNextWindowSize(ImVec2(300, -1));
        ImGui::Begin("LOD Editor", &visible, ImGuiWindowFlags_NoCollapse);
        ImGui::PushItemWidth(-1);

        constexpr float panelHeight = 250.0f;
        ImGui::BeginChild("ScrollableRegion", ImVec2(0, panelHeight), ImGuiChildFlags_AutoResizeY);
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
            const float buttonWidth = space.x / 2.0f;
            if (ImGui::Button(std::format("Export##{}", lodIndex).c_str(), ImVec2(buttonWidth, 0)))
            {
                SDL_ShowSaveFileDialog(saveLodCallback, &lod, window, DialogFilters::objFilters.data(), 1, nullptr);
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

        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        if (ImGui::Button("Add", ImVec2(60, 0)))
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
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x - 60, 0));
        ImGui::SameLine();
        if (ImGui::Button("OK", ImVec2(60, 0)))
        {
            if (!ModelRenderer::GetModel().ValidateLodDistances())
            {
                if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING,
                                              "Warning",
                                              "LOD distances are invalid! Make sure that:\n"
                                              "- The first LOD (LOD 0) has a distance of 0\n"
                                              "- No two LODs have the same distance",
                                              window))
                {
                    printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
                    return;
                }
            }
            visible = false;
        }

        ImGui::End();
    }
}

void LodEditWindow::addLodCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
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

void LodEditWindow::saveLodCallback(void *userdata, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    static_cast<ModelLod *>(userdata)->Export(fileList[0]);
}
