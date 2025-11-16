//
// Created by droc101 on 11/16/25.
//

#include "MaterialBrowserWindow.h"
#include <cstddef>
#include <format>
#include <imgui.h>
#include <libassets/util/Error.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>
#include "libassets/asset/LevelMaterialAsset.h"
#include "Options.h"
#include "SharedMgr.h"

constexpr int tileSize = 128;
static std::string filter = "";

void MaterialBrowserWindow::Hide()
{
    visible = false;
}

void MaterialBrowserWindow::Show(std::string &material)
{
    str = &material;
    materialPaths = SharedMgr::ScanFolder(Options::gamePath + "/assets/material", ".gmtl", true);
    for (const std::string &path: materialPaths)
    {
        LevelMaterialAsset mat;
        Error::ErrorCode e = LevelMaterialAsset::CreateFromAsset((Options::gamePath + "/assets/material/" + path).c_str(), mat);
        materials.push_back(mat);
    }
    visible = true;
}

void MaterialBrowserWindow::Render()
{
    if (visible)
    {
        ImGui::OpenPopup("Choose Material");
        ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Appearing);
        ImGui::SetNextWindowSizeConstraints(ImVec2(192, 192), ImVec2(FLT_MAX, FLT_MAX));
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse |
                                                 ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::BeginPopupModal("Choose Material", &visible, windowFlags))
        {
            ImGui::PushItemWidth(-1);
            ImGui::InputTextWithHint("##search", "Filter", &filter);
            ImGui::Dummy({0,4});
            if (ImGui::BeginChild("##picker", ImVec2(-1, -32), ImGuiChildFlags_Border, 0))
            {
                const float spacing = ImGui::GetStyle().ItemSpacing.x;
                const float regionMaxX = ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x;
                bool foundResults = false;

                for (size_t i = 0; i < materialPaths.size(); i++)
                {
                    if (materialPaths.at(i).find(filter) == std::string::npos) {continue;}

                    const std::string textureName = materials.at(i).texture;

                    ImVec2 texSize;
                    if (SharedMgr::textureCache->GetTextureSize(textureName, texSize) !=
                        Error::ErrorCode::OK)
                    {
                        continue;
                    }
                    ImTextureID tex = 0;
                    if (SharedMgr::textureCache->GetTextureID(textureName, tex) != Error::ErrorCode::OK)
                    {
                        continue;
                    }

                    ImGui::PushID(static_cast<int>(i));

                    const ImVec2 pos = ImGui::GetCursorScreenPos();
                    if (pos.x + tileSize > regionMaxX && i > 0)
                    {
                        ImGui::NewLine();
                    }

                    const float cursor = ImGui::GetCursorPosX();

                    if (ImGui::Selectable("##tile", "material/" + materialPaths[i] == *str, 0, ImVec2(tileSize, tileSize)))
                    {
                        *str = "material/" + materialPaths[i];
                    }
                    if (ImGui::BeginItemTooltip())
                    {
                        const std::string tooltip = materialPaths[i];
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                        ImGui::TextUnformatted(tooltip.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(cursor);


                    const float aspect = texSize.x / texSize.y;
                    float drawWidth = 0.0f;
                    float drawHeight = 0.0f;
                    if (aspect > 1.0f)
                    {
                        drawWidth = tileSize;
                        drawHeight = tileSize / aspect;
                    } else
                    {
                        drawWidth = tileSize * aspect;
                        drawHeight = tileSize;
                    }

                    ImVec2 cursorPos = ImGui::GetCursorPos();
                    ImGui::SetCursorPos(ImVec2(cursorPos.x + (tileSize - drawWidth) * 0.5f,
                                               cursorPos.y + (tileSize - drawHeight) * 0.5f));

                    ImGui::Image(tex, ImVec2(drawWidth, drawHeight));
                    cursorPos = ImGui::GetCursorPos();

                    ImGui::SameLine(0.0f, spacing);
                    ImGui::SetCursorPosX(cursor + tileSize + spacing);
                    ImGui::Dummy(ImVec2(0,0));
                    ImGui::SameLine(0,0);
                    ImGui::PopID();

                    foundResults = true;
                }

                if (!foundResults)
                {
                    ImGui::Text("No results");
                }

                ImGui::EndChild();
            }
            ImGui::Dummy({0,4});

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

void MaterialBrowserWindow::InputMaterial(const char *label, std::string &material)
{
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::InputText(label, &material);
    ImGui::SameLine();
    if (ImGui::Button(("..." + std::string(label)).c_str(), ImVec2(40, 0)))
    {
        Show(material);
    }
}

