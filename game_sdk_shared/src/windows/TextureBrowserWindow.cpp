//
// Created by droc101 on 7/6/25.
//

#include <cstddef>
#include <format>
#include <game_sdk/Options.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/windows/TextureBrowserWindow.h>
#include <imgui.h>
#include <libassets/util/Error.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>

constexpr int tileSize = 128;
static std::string filter = "";

void TextureBrowserWindow::Hide()
{
    visible = false;
}

void TextureBrowserWindow::Show(std::string &texture)
{
    str = &texture;
    textures = SharedMgr::ScanFolder(Options::GetAssetsPath() + "/texture", ".gtex", true);
    visible = true;
}

void TextureBrowserWindow::Render()
{
    if (visible)
    {
        ImGui::OpenPopup("Choose Texture");
        ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Appearing);
        ImGui::SetNextWindowSizeConstraints(ImVec2(192, 192), ImVec2(FLT_MAX, FLT_MAX));
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse |
                                                 ImGuiWindowFlags_NoSavedSettings |
                                                 ImGuiWindowFlags_NoDocking;
        if (ImGui::BeginPopupModal("Choose Texture", &visible, windowFlags))
        {
            ImGui::PushItemWidth(-1);
            ImGui::InputTextWithHint("##search", "Filter", &filter);
            ImGui::Dummy({0, 4});
            if (ImGui::BeginChild("##picker", ImVec2(-1, -32), ImGuiChildFlags_Borders, 0))
            {
                const float spacing = ImGui::GetStyle().ItemSpacing.x;
                const float regionMaxX = ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x;
                bool foundResults = false;

                for (size_t i = 0; i < textures.size(); i++)
                {
                    if (textures.at(i).find(filter) == std::string::npos)
                    {
                        continue;
                    }

                    ImVec2 texSize;
                    if (SharedMgr::textureCache.GetTextureSize("texture/" + textures[i], texSize) !=
                        Error::ErrorCode::OK)
                    {
                        continue;
                    }
                    ImTextureID tex = 0;
                    if (SharedMgr::textureCache.GetTextureID("texture/" + textures[i], tex) != Error::ErrorCode::OK)
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

                    if (ImGui::Selectable("##tile", "texture/" + textures[i] == *str, 0, ImVec2(tileSize, tileSize)))
                    {
                        *str = "texture/" + textures[i];
                    }
                    if (ImGui::BeginItemTooltip())
                    {
                        const std::string tooltip = std::format("{}\n{}x{}", textures[i], texSize.x, texSize.y);
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
                    ImGui::Dummy(ImVec2(0, 0));
                    ImGui::SameLine(0, 0);
                    ImGui::PopID();

                    foundResults = true;
                }

                if (!foundResults)
                {
                    ImGui::Text("No results");
                }

                ImGui::EndChild();
            }
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

void TextureBrowserWindow::InputTexture(const char *label, std::string &texture)
{
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::InputText(label, &texture);
    ImGui::SameLine();
    if (ImGui::Button(("..." + std::string(label)).c_str(), ImVec2(40, 0)))
    {
        Show(texture);
    }
}
