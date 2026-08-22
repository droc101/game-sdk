//
// Created by droc101 on 11/12/25.
//

#include <array>
#include <cstdlib>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/Options.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/Window.h>
#include <game_sdk/WindowManager.h>
#include <game_sdk/windows/SetupWindow.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>

SetupWindow::SetupWindow(const bool required)
{
    this->required = required;
}

const Window::WindowProperties &SetupWindow::GetProperties() const
{
    return properties;
}

void SetupWindow::GamePathCallback(const std::string &path)
{
    Options::Get().gameExecutablePath = path;
}

void SetupWindow::AssetsPathCallback(const std::string &path)
{
    Options::Get().gameConfigPath = path;
}

void SetupWindow::Render()
{
    ImGui::PushFont(normalFont, 24);
    ImGui::Text("GAME SDK Setup");
    ImGui::PopFont();
    ImGui::Separator();

    ImGui::TextUnformatted("GAME executable path");
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::InputText("##gamepathinput", &Options::Get().gameExecutablePath);
    ImGui::SameLine();
    if (ImGui::Button("...", ImVec2(40, 0)))
    {
        OpenFileDialog(GamePathCallback, DialogFilters::EXE_FILTERS);
    }

    ImGui::Text("Game configuration Path");
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::InputText("##assetspathinput", &Options::Get().gameConfigPath);
    ImGui::SameLine();
    if (ImGui::Button("...##assets", ImVec2(40, 0)))
    {
        OpenFileDialog(AssetsPathCallback, DialogFilters::GKVL_FILTERS);
    }

    bool valid = true;
    if (!Options::Get().ValidateGamePath())
    {
        valid = false;
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid executable or config path");
    } else
    {
        ImGui::Text(" ");
    }

    ImGui::TextUnformatted("Theme");
    ImGui::PushItemWidth(-1);
    int theme = static_cast<int>(Options::Get().theme);
    constexpr std::array<const char *, 3> THEME_OPTIONS = {"System", "Light", "Dark"};
    if (ImGui::Combo("##theme", &theme, THEME_OPTIONS.data(), 3))
    {
        Options::Get().theme = static_cast<Options::Theme>(theme);
        WindowManager::Get().ApplyTheme();
    }


    ImGui::Dummy(ImVec2(0, 16));
    const float sizeX = ImGui::GetContentRegionAvail().x;

    ImGui::Dummy(ImVec2(sizeX - 120 - ImGui::GetStyle().WindowPadding.x - ImGui::GetStyle().WindowPadding.x, 0));
    ImGui::SameLine();
    ImGui::BeginDisabled(!valid);
    if (ImGui::Button("OK", ImVec2(60, 0)))
    {
        Options::Get().Save();
        SharedMgr::Get().UpdateAssetPaths();
        closeRequest = true;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (required)
    {
        if (ImGui::Button("Quit", ImVec2(60, 0)))
        {
            Options::Get().Load();
            SharedMgr::Get().UpdateAssetPaths();
            exit(1);
        }
    } else
    {
        if (ImGui::Button("Cancel", ImVec2(60, 0)))
        {
            Options::Get().Load();
            SharedMgr::Get().UpdateAssetPaths();
            closeRequest = true;
        }
    }
}
