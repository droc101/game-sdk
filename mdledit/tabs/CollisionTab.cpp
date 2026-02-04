//
// Created by droc101 on 8/20/25.
//

#include "CollisionTab.h"
#include <cstddef>
#include <format>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SDKWindow.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <libassets/type/ConvexHull.h>
#include <string>
#include "../ModelRenderer.h"

void importSingleHull(const std::string &path);
void importMultipleHulls(const std::string &path);
void importStaticCollider(const std::string &path);

void CollisionTab::Render()
{
    ModelAsset &model = ModelRenderer::GetModel();
    ImGui::Begin("Collision", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Bounding Box");
    ImGui::InputFloat3("Origin", glm::value_ptr(model.GetBoundingBox().origin));
    ImGui::InputFloat3("Extents", glm::value_ptr(model.GetBoundingBox().extents));
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
        RenderCHullUI();
    } else if (model.GetCollisionModelType() == ModelAsset::CollisionModelType::STATIC_SINGLE_CONCAVE)
    {
        RenderStaticMeshUI();
    }

    ImGui::End();
}

void CollisionTab::RenderStaticMeshUI()
{
    ModelAsset &model = ModelRenderer::GetModel();
    if (ImGui::Button("Import Collision Mesh"))
    {
        SDKWindow::OpenFileDialog(importStaticCollider, DialogFilters::modelFilters);
    }
    ImGui::Text("%zu triangles", model.GetStaticCollisionMesh().GetNumTriangles());
}


void CollisionTab::RenderCHullUI()
{
    if (ImGui::Button("Import Hull"))
    {
        SDKWindow::OpenFileDialog(importSingleHull, DialogFilters::modelFilters);
    }
    ImGui::SameLine();
    if (ImGui::Button("Import Hulls"))
    {
        SDKWindow::OpenFileDialog(importMultipleHulls, DialogFilters::modelFilters);
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
