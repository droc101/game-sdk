//
// Created by droc101 on 8/21/26.
//

#include "FoneditWindow.h"
#include <cstddef>
#include <cstdint>
#include <format>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/Window.h>
#include <game_sdk/WindowManager.h>
#include <game_sdk/windows/TextureBrowserWindow.h>
#include <imgui.h>
#include <libassets/util/Error.h>
#include <libassets/util/VectorMove.h>
#include <string>
#include <vector>

void FoneditWindow::OpenGfon(const std::string &path)
{
    const Error::ErrorCode errorCode = font.LoadFromAsset(path);
    if (errorCode != Error::ErrorCode::OK)
    {
        ErrorMessage(std::format("Failed to open the font!\n{}", errorCode));
    }
}

void FoneditWindow::SaveGfon(const std::string &path)
{
    const Error::ErrorCode errorCode = font.SaveToAsset(path);
    if (errorCode != Error::ErrorCode::OK)
    {
        ErrorMessage(std::format("Failed to save the font!\n{}", errorCode));
    }
}

const char *FoneditWindow::ComboGetter(void *user_data, int index)
{
    const std::vector<std::string> &items = *static_cast<std::vector<std::string> *>(user_data);
    if (index < 0 || static_cast<size_t>(index) >= items.size())
    {
        return nullptr;
    }
    return items.at(index).c_str();
}

void FoneditWindow::Render()
{
    bool newPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S);

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            newPressed |= ImGui::MenuItem("New", "Ctrl+N");
            ImGui::Separator();
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S");
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                RequestClose();
            }
            ImGui::EndMenu();
        }
        SharedMgr::Get().SharedMenuUI("fonedit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        OpenFileDialog(FILE_DIALOG_CALLBACK(OpenGfon), DialogFilters::GFON_FILTERS);
    } else if (savePressed)
    {
        SaveFileDialog(FILE_DIALOG_CALLBACK(SaveGfon), DialogFilters::GFON_FILTERS);
    } else if (newPressed)
    {
        font = FontAsset();
    }

    const ImVec2 &availableSize = ImGui::GetContentRegionAvail();

    constexpr float SIDEBAR_WIDTH = 300.0f;
    const float imageWidth = availableSize.x - SIDEBAR_WIDTH - 8.0f;

    ImGui::BeginChild("ImagePane",
                      ImVec2(imageWidth, availableSize.y),
                      ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
    {
        if (ImGui::Button("Add Symbol"))
        {
            if (!font.chars.empty())
            {
                font.chars.push_back(static_cast<char>(font.chars.back() + 1));
            } else
            {
                font.chars.push_back('a');
            }

            font.charWidths.push_back(font.defaultSize);
        }
        const ImVec2 availSize = ImGui::GetContentRegionAvail();
        if (ImGui::BeginTable("charTable", 4, ImGuiTableFlags_ScrollY, availSize))
        {
            ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 40);
            ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Display Width", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Controls",
                                    ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed,
                                    (40 * 3) + (ImGui::GetStyle().WindowPadding.x * 3));
            ImGui::TableHeadersRow();
            for (size_t i = 0; i < font.chars.size(); i++)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(std::format("{}", i).c_str());
                ImGui::TableNextColumn();
                int charIndex = static_cast<int>(FontAsset::FONT_VALID_CHARS.find(font.chars.at(i)));
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo(std::format("##char_{}", i).c_str(),
                                 &charIndex,
                                 ComboGetter,
                                 &charDisplayList,
                                 static_cast<int>(charDisplayList.size())))
                {
                    font.chars.at(i) = FontAsset::FONT_VALID_CHARS.at(charIndex);
                }
                ImGui::TableNextColumn();
                int width = font.charWidths.at(i);
                ImGui::PushItemWidth(-1);
                if (ImGui::SliderInt(std::format("##CharWidth_{}", i).c_str(), &width, 1, 32, "%d px"))
                {
                    font.charWidths.at(i) = width;
                }
                ImGui::TableNextColumn();
                if (ImGui::Button(std::format("Up##{}", i).c_str(), ImVec2(40, 0)))
                {
                    MoveBack<uint8_t>(font.charWidths, i);
                    MoveBack<char>(font.chars, i);
                }
                ImGui::SameLine();
                if (ImGui::Button(std::format("Down##{}", i).c_str(), ImVec2(40, 0)))
                {
                    MoveForward<uint8_t>(font.charWidths, i);
                    MoveForward<char>(font.chars, i);
                }
                ImGui::SameLine();
                if (ImGui::Button(std::format("Del##{}", i).c_str(), ImVec2(40, 0)))
                {
                    font.charWidths.erase(font.charWidths.begin() + static_cast<ptrdiff_t>(i));
                    font.chars.erase(font.chars.begin() + static_cast<ptrdiff_t>(i));
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("StatsPane", ImVec2(SIDEBAR_WIDTH, availableSize.y));
    {
        int charWidth = font.charWidth;
        int textureHeight = font.textureHeight;
        int baseline = font.baseline;
        int charSpacing = font.charSpacing;
        int lineSpacing = font.lineSpacing;
        int spaceWidth = font.spaceWidth;
        int defaultSize = font.defaultSize;

        ImGui::PushItemWidth(-1);
        ImGui::TextUnformatted("Char slot width in texture");
        if (ImGui::SliderInt("##charWidth", &charWidth, 1, 32))
        {
            font.charWidth = charWidth;
        }
        ImGui::TextUnformatted("Texture Height");
        if (ImGui::SliderInt("##textureHeight", &textureHeight, 1, 32))
        {
            font.textureHeight = textureHeight;
        }
        ImGui::TextUnformatted("Baseline");
        if (ImGui::SliderInt("##baseline", &baseline, 1, 32))
        {
            font.baseline = baseline;
        }
        ImGui::TextUnformatted("Symbol Spacing");
        if (ImGui::SliderInt("##charSpacing", &charSpacing, 0, 8))
        {
            font.charSpacing = charSpacing;
        }
        ImGui::TextUnformatted("Line Spacing");
        if (ImGui::SliderInt("##lineSpacing", &lineSpacing, 0, 8))
        {
            font.lineSpacing = lineSpacing;
        }
        ImGui::TextUnformatted("Space Width");
        if (ImGui::SliderInt("##spaceWidth", &spaceWidth, 0, 32))
        {
            font.spaceWidth = spaceWidth;
        }
        ImGui::TextUnformatted("Default Font Size");
        if (ImGui::SliderInt("##defaultSize", &defaultSize, 0, 32))
        {
            font.defaultSize = defaultSize;
        }
        ImGui::TextUnformatted("Font Texture");
        TextureBrowserWindow::InputTexture("##Texture", font.texture);
        ImGui::Checkbox("Uppercase only", &font.uppercaseOnly);
    }
    ImGui::EndChild();
}

const Window::WindowProperties &FoneditWindow::GetProperties() const
{
    return properties;
}

bool FoneditWindow::Init()
{
    charDisplayList = FontAsset::GetCharListForDisplay();

    const std::string &openPath = WindowManager::Get().GetArgumentParser().GetFileArgument({".gfon"});
    if (!openPath.empty())
    {
        OpenGfon(openPath);
    }

    return true;
}
