//
// Created by droc101 on 7/6/25.
//

#include "TextureBrowserWindow.h"
#include <format>
#include "imgui.h"
#include "Options.h"
#include "SharedMgr.h"
#include "libassets/util/Error.h"
#include "misc/cpp/imgui_stdlib.h"

constexpr float tileSize = 128;

void TextureBrowserWindow::Hide()
{
    visible = false;
}

void TextureBrowserWindow::Show(std::string &texture)
{
    str = &texture;
    textures = SharedMgr::ScanFolder(Options::gamePath + "/assets/texture", ".gtex");
    visible = true;
}

void TextureBrowserWindow::Render()
{
    if (visible)
    {
        ImGui::OpenPopup("Choose Texture");
        ImGui::SetNextWindowSize(ImVec2(600, -1), ImGuiCond_Appearing);
        if (ImGui::BeginPopupModal("Choose Texture",
                                   &visible,
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoResize))
        {
            if (ImGui::BeginChild("##picker", ImVec2(-1, 400), ImGuiChildFlags_Border, 0))
            {
                const float spacing = ImGui::GetStyle().ItemSpacing.x;
                const float regionMaxX = ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x;

                for (size_t i = 0; i < textures.size(); ++i)
                {
                    ImGui::PushID(static_cast<int>(i));

                    const ImVec2 pos = ImGui::GetCursorScreenPos();
                    if (pos.x + tileSize > regionMaxX && i > 0)
                        ImGui::NewLine();

                    const float cursor = ImGui::GetCursorPosX();

                    if (ImGui::Selectable("##tile", ("texture/" + textures[i]) == *str, 0, ImVec2(tileSize, tileSize)))
                    {
                        *str = "texture/" + textures[i];
                    }
                    ImVec2 texSize;
                    if (SharedMgr::textureCache->GetTextureSize("texture/" + textures[i], texSize) != Error::ErrorCode::E_OK) continue;
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
                    float draw_width;
                    float draw_height;
                    if (aspect > 1.0f)
                    {
                        draw_width = tileSize;
                        draw_height = tileSize / aspect;
                    } else
                    {
                        draw_width = tileSize * aspect;
                        draw_height = tileSize;
                    }

                    const ImVec2 cursor_pos = ImGui::GetCursorPos();
                    ImGui::SetCursorPos(ImVec2(
                            cursor_pos.x + (tileSize - draw_width) * 0.5f,
                            cursor_pos.y + (tileSize - draw_height) * 0.5f
                            ));

                    ImTextureID tex;
                    if (SharedMgr::textureCache->GetTextureID("texture/" + textures[i], tex) != Error::ErrorCode::E_OK) continue;
                    ImGui::Image(tex, ImVec2(draw_width, draw_height));
                    ImGui::SetCursorPosX(-draw_width);
                    ImGui::SetCursorPosX(tileSize);

                    ImGui::SameLine(0.0f, spacing);
                    ImGui::PopID();
                }

                ImGui::EndChild();
            }

            const float sizeX = ImGui::GetContentRegionAvail().x;

            ImGui::Dummy(ImVec2(sizeX - 60 - ImGui::GetStyle().WindowPadding.x, 0));
            ImGui::SameLine();
            if (ImGui::Button("OK", ImVec2(60, 0)))
            {
                visible = false;
            }
            // ImGui::SameLine();
            // if (ImGui::Button("Cancel", ImVec2(60, 0)))
            // {
            //     visible = false;
            // }

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
