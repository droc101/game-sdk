//
// Created by droc101 on 2/17/26.
//

#include <cassert>
#include <game_sdk/ModelViewer.h>
#include <game_sdk/Options.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/windows/ModelBrowserWindow.h>
#include <imgui.h>
#include <libassets/asset/ModelAsset.h>
#include <libassets/util/Error.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>
#include <utility>
#include <vector>

constexpr int tileSize = 256;
static std::string filter = "";

ModelBrowserWindow &ModelBrowserWindow::Get()
{
    static ModelBrowserWindow modelBrowserWindowSingleton{};
    return modelBrowserWindowSingleton;
}

void ModelBrowserWindow::Hide()
{
    visible = false;
}

void ModelBrowserWindow::Show(std::string *model)
{
    str = model;
    (void)viewer.Init();
    models = SharedMgr::Get().ScanFolder(Options::Get().GetAssetsPath() + "/model", ".gmdl", true);
    ModelAsset mdlAsset{};
    (void)ModelAsset::CreateFromAsset(Options::Get().GetAssetsPath() + "/" + *model, mdlAsset);
    viewer.SetModel(std::move(mdlAsset));
    visible = true;
}

void ModelBrowserWindow::InputModel(const char *label, std::string &model)
{
    InputModel(label, &model);
}

void ModelBrowserWindow::InputModel(const char *label, std::string *model)
{
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::InputText(label, model);
    ImGui::SameLine();
    if (ImGui::Button(("..." + std::string(label)).c_str(), ImVec2(40, 0)))
    {
        Show(model);
    }
}

void ModelBrowserWindow::Render()
{
    if (visible)
    {
        ImGui::OpenPopup("Choose Model");
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
        ImGui::SetNextWindowSizeConstraints(ImVec2(300, 192), ImVec2(FLT_MAX, FLT_MAX));
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse |
                                                 ImGuiWindowFlags_NoSavedSettings |
                                                 ImGuiWindowFlags_NoDocking;
        if (ImGui::BeginPopupModal("Choose Model", &visible, windowFlags))
        {
            ImGui::PushItemWidth(-1);
            ImGui::InputTextWithHint("##search", "Filter", &filter);
            ImGui::Dummy({0, 4});
            bool foundResults = false;
            if (ImGui::BeginListBox("##models", {200, -36}))
            {
                for (const std::string &model: models)
                {
                    if (model.find(filter) == std::string::npos)
                    {
                        continue;
                    }

                    if (ImGui::Selectable(model.c_str(), "model/" + model == *str))
                    {
                        if (*str != "model/" + model)
                        {
                            ModelAsset mdlAsset{};
                            assert(ModelAsset::CreateFromAsset(Options::Get().GetAssetsPath() + "/model/" + model,
                                                               mdlAsset) == Error::ErrorCode::OK);
                            viewer.SetModel(std::move(mdlAsset));
                        }
                        *str = "model/" + model;
                    }

                    foundResults = true;
                }

                if (!foundResults)
                {
                    ImGui::Text("No Results");
                }

                ImGui::EndListBox();
            }

            ImGui::SameLine();
            viewer.RenderChildWindow("##preview", {-1, -36}, 0, 0);

            ImGui::Dummy({0, 4});

            const float sizeX = ImGui::GetContentRegionAvail().x;

            ImGui::Dummy(ImVec2(sizeX - 60 - ImGui::GetStyle().WindowPadding.x, 0));
            ImGui::SameLine();
            if (ImGui::Button("OK", ImVec2(60, 0)))
            {
                visible = false;
            }

            ImGui::EndPopup();
        }
    }
}
