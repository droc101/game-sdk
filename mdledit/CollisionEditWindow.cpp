//
// Created by droc101 on 8/20/25.
//

#include "CollisionEditWindow.h"
#include <cstddef>
#include <format>
#include <imgui.h>
#include <libassets/util/ConvexHull.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <string>
#include "DialogFilters.h"
#include "ModelRenderer.h"

void CollisionEditWindow::Show()
{
    visible = true;
}

void CollisionEditWindow::Hide()
{
    visible = false;
}

void CollisionEditWindow::Render(SDL_Window *window)
{
    if (visible)
    {
        ModelAsset &model = ModelRenderer::GetModel();
        ImGui::SetNextWindowSize(ImVec2(300, -1));
        ImGui::Begin("Collision Editor", &visible, ImGuiWindowFlags_NoCollapse);
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
                               model.GetCollisionModelType() ==
                               ModelAsset::CollisionModelType::DYNAMIC_MULTIPLE_CONVEX))
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
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x - 60, 0));
        ImGui::SameLine();
        if (ImGui::Button("OK", ImVec2(60, 0)))
        {
            visible = false;
        }

        ImGui::End();
    }
}

void CollisionEditWindow::RenderStaticMeshUI(SDL_Window *window)
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


void CollisionEditWindow::RenderCHullUI(SDL_Window *window)
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
    ImGui::BeginChild("ScrollableRegion", ImVec2(0, panelHeight), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border);
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

void CollisionEditWindow::ImportStaticMeshCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
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

void CollisionEditWindow::AddSingleHullCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
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

void CollisionEditWindow::AddMultipleHullsCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
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
