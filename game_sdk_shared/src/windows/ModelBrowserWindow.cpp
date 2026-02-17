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
        ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_Appearing);
        ImGui::SetNextWindowSizeConstraints(ImVec2(300, 192), ImVec2(FLT_MAX, FLT_MAX));
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse |
                                                 ImGuiWindowFlags_NoSavedSettings |
                                                 ImGuiWindowFlags_NoDocking;
        if (ImGui::BeginPopupModal("Choose Model", &visible, windowFlags))
        {
            ImGui::PushItemWidth(200);
            const float cursorTopY = ImGui::GetCursorPosY();
            ImGui::InputTextWithHint("##search", "Filter", &filter);
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
            const float cursorX = ImGui::GetCursorPosX();
            ImGui::SetCursorPosY(cursorTopY);
            viewer.RenderChildWindow("##preview", {-1, -36 - 250}, 0, 0);

            ImGui::SetCursorPosX(cursorX);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 250 - 4);
            if (ImGui::BeginChild("##options", {-1, 250}, ImGuiChildFlags_Borders))
            {
                ImGui::PushItemWidth(-1);
                if (ImGui::BeginTable("##skinLodGrid", 2))
                {
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("LOD");
                    ImGui::PushItemWidth(-1);
                    ImGui::SliderInt("##LOD", &viewer.lodIndex, 0, static_cast<int>(viewer.GetModel().GetLodCount() - 1));

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Skin");
                    ImGui::PushItemWidth(-1);
                    ImGui::SliderInt("##Skin",
                                     &viewer.skinIndex,
                                     0,
                                     static_cast<int>(viewer.GetModel().GetSkinCount() - 1));
                    ImGui::EndTable();
                }
                ImGui::SeparatorText("Display Options");
                if (ImGui::BeginTable("##optionsGrid", 5))
                {
                    ImGui::TableNextColumn();
                    ImGui::Checkbox("Wireframe", &viewer.wireframe);
                    ImGui::TableNextColumn();
                    ImGui::Checkbox("Cull Backfaces", &viewer.cullBackfaces);
                    ImGui::TableNextColumn();
                    ImGui::Checkbox("Unit Cube", &viewer.showUnitCube);
                    ImGui::TableNextColumn();
                    ImGui::Checkbox("Bounding Box", &viewer.showBoundingBox);
                    ImGui::TableNextColumn();
                    ImGui::Checkbox("Collision Model", &viewer.showCollisionModel);
                    ImGui::EndTable();
                }

                ImGui::Text("Background Color");
                ImGui::ColorEdit3("##bgColor", viewer.backgroundColor.GetDataPointer());

                ImGui::SeparatorText("Display Mode");
                if (ImGui::BeginTable("##optionsGrid", 3))
                {
                    ImGui::TableNextColumn();
                    if (ImGui::RadioButton("Unshaded", viewer.displayMode == ModelViewer::DisplayMode::COLORED))
                    {
                        viewer.displayMode = ModelViewer::DisplayMode::COLORED;
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::RadioButton("Shaded", viewer.displayMode == ModelViewer::DisplayMode::COLORED_SHADED))
                    {
                        viewer.displayMode = ModelViewer::DisplayMode::COLORED_SHADED;
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::RadioButton("Textured Unshaded", viewer.displayMode == ModelViewer::DisplayMode::TEXTURED))
                    {
                        viewer.displayMode = ModelViewer::DisplayMode::TEXTURED;
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::RadioButton("Textured Shaded",
                                           viewer.displayMode == ModelViewer::DisplayMode::TEXTURED_SHADED))
                    {
                        viewer.displayMode = ModelViewer::DisplayMode::TEXTURED_SHADED;
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::RadioButton("UV Debug", viewer.displayMode == ModelViewer::DisplayMode::UV))
                    {
                        viewer.displayMode = ModelViewer::DisplayMode::UV;
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::RadioButton("Normal Debug", viewer.displayMode == ModelViewer::DisplayMode::NORMAL))
                    {
                        viewer.displayMode = ModelViewer::DisplayMode::NORMAL;
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::EndChild();

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
