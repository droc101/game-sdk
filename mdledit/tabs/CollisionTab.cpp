//
// Created by droc101 on 8/20/25.
//

#include "CollisionTab.h"
#include <cstddef>
#include <format>
#include <game_sdk/DialogFilters.h>
#include <imgui.h>
#include <libassets/type/ConvexHull.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <string>
#include "../ModelRenderer.h"

void CollisionTab::Render(SDL_Window *window)
{
    ModelAsset &model = ModelRenderer::GetModel();
    ImGui::Begin("Collision", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Bounding Box");
    ImGui::InputFloat3("Origin", model.GetBoundingBox().origin.data());
    ImGui::InputFloat3("Extents", model.GetBoundingBox().extents.data());
    if (ImGui::Button("Autocalculate"))
    {
        model.GetBoundingBox() = BoundingBox(model.GetLod(0).vertices);
    }
    ImGui::Separator();
    ImGui::Text("Collision Model Type:");
    if (ImGui::RadioButton("None", model.GetCollisionModelType() == ModelAsset::CollisionModelType::NONE))
    {
        model.GetCollisionModelType() = ModelAsset::CollisionModelType::NONE;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Static",
                           model.GetCollisionModelType() == ModelAsset::CollisionModelType::STATIC_SINGLE_CONCAVE))
    {
        model.GetCollisionModelType() = ModelAsset::CollisionModelType::STATIC_SINGLE_CONCAVE;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Static collision models are a single arbitrary mesh that is allowed to be "
                          "concave.\nThis model type can not be used on moving actors.");
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Dynamic",
                           model.GetCollisionModelType() == ModelAsset::CollisionModelType::DYNAMIC_MULTIPLE_CONVEX))
    {
        model.GetCollisionModelType() = ModelAsset::CollisionModelType::DYNAMIC_MULTIPLE_CONVEX;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Dynamic collision models are a collection of convex hulls.\nConcave shapes can only be "
                          "created using multiple hulls.\nIt is up to you to ensure the imported hulls are "
                          "actually convex.");
    }

    if (model.GetCollisionModelType() == ModelAsset::CollisionModelType::DYNAMIC_MULTIPLE_CONVEX)
    {
        RenderCHullUI(window);
    } else if (model.GetCollisionModelType() == ModelAsset::CollisionModelType::STATIC_SINGLE_CONCAVE)
    {
        RenderStaticMeshUI(window);
    }

    ImGui::End();
}

void CollisionTab::RenderStaticMeshUI(SDL_Window *window)
{
    ModelAsset &model = ModelRenderer::GetModel();
    if (ImGui::Button("Import Collision Mesh"))
    {
        SDL_ShowOpenFileDialog(ImportStaticMeshCallback,
                               nullptr,
                               window,
                               DialogFilters::modelFilters.data(),
                               DialogFilters::modelFilters.size(),
                               nullptr,
                               false);
    }
    ImGui::Text("%zu triangles", model.GetStaticCollisionMesh().GetNumTriangles());
}


void CollisionTab::RenderCHullUI(SDL_Window *window)
{
    if (ImGui::Button("Import Hull"))
    {
        SDL_ShowOpenFileDialog(AddSingleHullCallback,
                               nullptr,
                               window,
                               DialogFilters::modelFilters.data(),
                               DialogFilters::modelFilters.size(),
                               nullptr,
                               false);
    }
    ImGui::SameLine();
    if (ImGui::Button("Import Hulls"))
    {
        SDL_ShowOpenFileDialog(AddMultipleHullsCallback,
                               nullptr,
                               window,
                               DialogFilters::modelFilters.data(),
                               DialogFilters::modelFilters.size(),
                               nullptr,
                               false);
    }
    constexpr float panelHeight = 250.0f;
    ImGui::BeginChild("ScrollableRegion",
                      ImVec2(0, panelHeight),
                      ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
    for (size_t hullIndex = 0; hullIndex < ModelRenderer::GetModel().GetNumHulls(); hullIndex++)
    {
        const std::string title = std::format("Shape {}", hullIndex);
        ImGui::SeparatorText(title.c_str());
        ConvexHull &hull = ModelRenderer::GetModel().GetHull(hullIndex);
        ImGui::TextUnformatted(std::format("{} points", hull.GetPoints().size()).c_str());
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
        const ImVec2 space = ImGui::GetContentRegionAvail();
        const float buttonWidth = space.x / 2.0f;
        if (ImGui::Button(std::format("Delete##{}", hullIndex).c_str(), ImVec2(buttonWidth, 0)))
        {
            ModelRenderer::GetModel().RemoveHull(hullIndex);
            ModelRenderer::LoadHulls();
        }
    }
    ImGui::EndChild();
}

void CollisionTab::ImportStaticMeshCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    SDL_Event event;
    event.type = ModelRenderer::EVENT_RELOAD_MODEL;
    event.user.code = ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_STATIC_COLLIDER;
    event.user.data1 = new std::string(fileList[0]);
    if (!SDL_PushEvent(&event))
    {
        printf("Error: SDL_PushEvent(): %s\n", SDL_GetError());
    }
}

void CollisionTab::AddSingleHullCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    SDL_Event event;
    event.type = ModelRenderer::EVENT_RELOAD_MODEL;
    event.user.code = ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_HULL;
    event.user.data1 = new std::string(fileList[0]);
    if (!SDL_PushEvent(&event))
    {
        printf("Error: SDL_PushEvent(): %s\n", SDL_GetError());
    }
}

void CollisionTab::AddMultipleHullsCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    SDL_Event event;
    event.type = ModelRenderer::EVENT_RELOAD_MODEL;
    event.user.code = ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_HULL_MULTI;
    event.user.data1 = new std::string(fileList[0]);
    if (!SDL_PushEvent(&event))
    {
        printf("Error: SDL_PushEvent(): %s\n", SDL_GetError());
    }
}
