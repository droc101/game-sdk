//
// Created by droc101 on 7/4/25.
//

#include "LodEditWindow.h"
#include <format>
#include <numeric>
#include <SDL3/SDL_dialog.h>
#include "imgui.h"
#include "ModelRenderer.h"

void LodEditWindow::Show()
{
    visible = true;
}

void LodEditWindow::Hide()
{
    visible = false;
}

void LodEditWindow::Render(SDL_Window * window)
{
    if (visible)
    {
        ImGui::SetNextWindowSize(ImVec2(300, -1));
        ImGui::Begin("LOD Editor",
                     &visible,
                     ImGuiWindowFlags_NoCollapse);
        ImGui::PushItemWidth(-1);

        constexpr float panelHeight = 250.0f;
        ImGui::BeginChild("ScrollableRegion", ImVec2(0, panelHeight), ImGuiChildFlags_AutoResizeY);
        for (size_t l = 0; l < ModelRenderer::GetModel()->GetLodCount(); l++)
        {
            const std::string title = std::format("LOD {}", l);
            ImGui::SeparatorText(title.c_str());
            ModelAsset::ModelLod &lod = ModelRenderer::GetModel()->GetLod(l);
            const uint tris = std::accumulate(lod.indexCounts.begin(), lod.indexCounts.end(), 0u) / 3u;
            ImGui::Text("%ld vertices\n%d triangles", lod.vertices.size(), tris);
            ImGui::InputFloat(std::format("##LOD_{}_Distance", l).c_str(), &lod.distance, 0.0f, 0.0f, "%.1f");
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                ModelRenderer::GetModel()->SortLODs();
            }
            const ImVec2 space = ImGui::GetContentRegionAvail();
            const float btnSizeX = space.x / 2.0f;
            if (ImGui::Button(std::format("Export##{}", l).c_str(), ImVec2(btnSizeX, 0)))
            {
                constexpr SDL_DialogFileFilter objFilter = {"Wavefront OBJ Model", "obj"};
                SDL_ShowSaveFileDialog(saveLodCallback, &lod, window, &objFilter, 1, nullptr);
            }
            if (ModelRenderer::GetModel()->GetLodCount() != 1)
            {
                ImGui::SameLine();
                if (ImGui::Button(std::format("Delete##{}", l).c_str(), ImVec2(btnSizeX, 0)))
                {
                    ModelAsset m = *ModelRenderer::GetModel();
                    m.RemoveLod(l);
                    ModelRenderer::LoadModel(m);
                }
            }
        }
        ImGui::EndChild();

        ImGui::Dummy(ImVec2(0.0f, 16.0f));
        if (ImGui::Button("Add", ImVec2(60, 0)))
        {
            constexpr std::array modelFilters = {
                SDL_DialogFileFilter{"3D Models (obj, fbx, gltf, dae)", "obj;fbx;gltf;dae"},
                SDL_DialogFileFilter{"Wavefront OBJ Models", "obj"},
                SDL_DialogFileFilter{"FBX Models", "fbx"},
                SDL_DialogFileFilter{"glTF/glTF2.0 Models", "gltf"},
                SDL_DialogFileFilter{"Collada Models", "dae"},
        };
            SDL_ShowOpenFileDialog(addLodCallback,
                               nullptr,
                               window,
                               modelFilters.data(),
                               modelFilters.size(),
                               nullptr,
                               false);
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

void LodEditWindow::addLodCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    ModelAsset m = *ModelRenderer::GetModel();
    m.AddLod(fileList[0]);
    ModelRenderer::LoadModel(m);
}

void LodEditWindow::saveLodCallback(void *userdata, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    static_cast<ModelAsset::ModelLod*>(userdata)->Export(fileList[0]);
}

