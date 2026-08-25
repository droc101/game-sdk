//
// Created by droc101 on 6/29/25.
//

#include <array>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/Options.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/Window.h>
#include <game_sdk/WindowManager.h>
#include <game_sdk/windows/MaterialBrowserWindow.h>
#include <game_sdk/windows/OptionsWindow.h>
#include <game_sdk/windows/TextureBrowserWindow.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>

void OptionsWindow::GamePathCallback(const std::string &path)
{
    Options::Get().gameExecutablePath = path;
}

void OptionsWindow::AssetsPathCallback(const std::string &path)
{
    Options::Get().gameConfigPath = path;
}

const Window::WindowProperties &OptionsWindow::GetProperties() const
{
    return properties;
}

void OptionsWindow::Render()
{
    ImGui::SeparatorText("GAME Installation");
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

    ImGui::SeparatorText("Default Assets");
    ImGui::TextUnformatted("Default Texture");
    TextureBrowserWindow::InputTexture("##defaulttexinput", Options::Get().defaultTexture);
    ImGui::TextUnformatted("Default Material");
    MaterialBrowserWindow::InputMaterial("##defaultmatinput", Options::Get().defaultMaterial);

    ImGui::SeparatorText("Appearance");
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
    if (ImGui::Button("OK", ImVec2(60, 0)))
    {
        Options::Get().Save();
        SharedMgr::Get().UpdateAssetPaths();
        RequestClose();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(60, 0)))
    {
        Options::Get().Load();
        SharedMgr::Get().UpdateAssetPaths();
        RequestClose();
    }
}
