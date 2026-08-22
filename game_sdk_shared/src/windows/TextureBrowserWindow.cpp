//
// Created by droc101 on 7/6/25.
//

#include <cfloat>
#include <cstddef>
#include <format>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/WindowManager.h>
#include <game_sdk/windows/TextureBrowserWindow.h>
#include <imgui.h>
#include <libassets/util/Error.h>
#include <memory>
#include <misc/cpp/imgui_stdlib.h>
#include <string>
#include <vector>

TextureBrowserWindow::TextureBrowserWindow(std::string *texture)
{
    str = texture;
}

bool TextureBrowserWindow::Init()
{
    textures.clear();
    textures = SharedMgr::Get().pathManager.ScanAssetFolderR("/texture", ".gtex");
    return true;
}

const Window::WindowProperties &TextureBrowserWindow::GetProperties() const
{
    return properties;
}

void TextureBrowserWindow::Render()
{
    ImGui::PushItemWidth(-1);
    ImGui::InputTextWithHint("##search", "Filter", &filter);
    ImGui::Dummy({0, 4});
    if (ImGui::BeginChild("##picker", ImVec2(-1, -36), ImGuiChildFlags_Borders, 0))
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
            ImTextureID tex = 0;
            if (SharedMgr::Get().textureCache.GetTextureID("texture/" + textures.at(i), tex) != Error::ErrorCode::OK)
            {
                texSize = SharedMgr::Get().textureCache.GetMissingTextureSize();
                tex = SharedMgr::Get().textureCache.GetMissingTextureID();
            } else
            {
                assert(SharedMgr::Get().textureCache.GetTextureSize("texture/" + textures.at(i), texSize) ==
                       Error::ErrorCode::OK);
            }

            ImGui::PushID(static_cast<int>(i));

            const ImVec2 pos = ImGui::GetCursorScreenPos();
            if (pos.x + TILE_SIZE > regionMaxX && i > 0)
            {
                ImGui::NewLine();
            }

            const float cursor = ImGui::GetCursorPosX();

            if (ImGui::Selectable("##tile", "texture/" + textures.at(i) == *str, 0, ImVec2(TILE_SIZE, TILE_SIZE)))
            {
                *str = "texture/" + textures.at(i);
            }
            if (ImGui::BeginItemTooltip())
            {
                const std::string tooltip = std::format("{}\n{}x{}", textures.at(i), texSize.x, texSize.y);
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
                drawWidth = TILE_SIZE;
                drawHeight = TILE_SIZE / aspect;
            } else
            {
                drawWidth = TILE_SIZE * aspect;
                drawHeight = TILE_SIZE;
            }

            const ImVec2 cursorPos = ImGui::GetCursorPos();
            ImGui::SetCursorPos(ImVec2(cursorPos.x + (TILE_SIZE - drawWidth) * 0.5f,
                                       cursorPos.y + (TILE_SIZE - drawHeight) * 0.5f));

            ImGui::Image(tex, ImVec2(drawWidth, drawHeight));

            ImGui::SameLine(0.0f, spacing);
            ImGui::SetCursorPosX(cursor + TILE_SIZE + spacing);
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
        closeRequest = true;
    }
}

void TextureBrowserWindow::InputTexture(const char *label, std::string &texture)
{
    InputTexture(label, &texture);
}

void TextureBrowserWindow::InputTexture(const char *label, std::string *texture)
{
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::PushFont(SDKWindow::Get().GetMonospaceFont());
    ImGui::InputText(label, texture);
    ImGui::PopFont();
    ImGui::SameLine();
    if (ImGui::Button(("..." + std::string(label)).c_str(), ImVec2(40, 0)))
    {
        Show(texture);
    }
}

void TextureBrowserWindow::Show(std::string *texture)
{
    WindowManager::Get().AddWindow(std::make_shared<TextureBrowserWindow>(texture));
}
