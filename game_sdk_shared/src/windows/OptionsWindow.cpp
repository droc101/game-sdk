//
// Created by droc101 on 6/29/25.
//

#include <array>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/Options.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/windows/MaterialBrowserWindow.h>
#include <game_sdk/windows/OptionsWindow.h>
#include <game_sdk/windows/TextureBrowserWindow.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>

OptionsWindow &OptionsWindow::Get()
{
    static OptionsWindow optionsWindowSingleton{};
    return optionsWindowSingleton;
}

void OptionsWindow::Show()
{
    visible = true;
}

void OptionsWindow::Hide()
{
    visible = false;
}

void OptionsWindow::gamePathCallback(const std::string &path)
{
    Options::Get().gameExecutablePath = path;
}

void OptionsWindow::assetsPathCallback(const std::string &path)
{
    Options::Get().gameConfigPath = path;
}

void OptionsWindow::Render()
{
    if (visible)
    {
        ImGui::SetNextWindowSize(ImVec2(450, -1));
        ImGui::Begin("GAME SDK Options",
                     &visible,
                     ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoDocking);
        ImGui::SeparatorText("GAME Installation");
        ImGui::TextUnformatted("GAME executable path");
        ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
        ImGui::InputText("##gamepathinput", &Options::Get().gameExecutablePath);
        ImGui::SameLine();
        if (ImGui::Button("...", ImVec2(40, 0)))
        {
            SDKWindow::Get().OpenFileDialog(gamePathCallback, DialogFilters::exeFilters);
        }

        ImGui::Text("Game Config Path");
        ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
        ImGui::InputText("##assetspathinput", &Options::Get().gameConfigPath);
        ImGui::SameLine();
        if (ImGui::Button("...##assets", ImVec2(40, 0)))
        {
            SDKWindow::Get().OpenFileDialog(assetsPathCallback, DialogFilters::gkvlFilters);
        }

        ImGui::SeparatorText("Default Assets");
        ImGui::TextUnformatted("Default Texture");
        TextureBrowserWindow::Get().InputTexture("##defaulttexinput", Options::Get().defaultTexture);
        ImGui::TextUnformatted("Default Material");
        MaterialBrowserWindow::Get().InputMaterial("##defaultmatinput", Options::Get().defaultMaterial);

        ImGui::SeparatorText("Appearance");
        ImGui::TextUnformatted("Theme");
        ImGui::PushItemWidth(-1);
        int theme = static_cast<int>(Options::Get().theme);
        constexpr std::array<const char *, 3> options = {"System", "Light", "Dark"};
        if (ImGui::Combo("##theme", &theme, options.data(), 3))
        {
            Options::Get().theme = static_cast<Options::Theme>(theme);
            SDKWindow::Get().ApplyTheme();
        }

        ImGui::Dummy(ImVec2(0, 16));

        const float sizeX = ImGui::GetContentRegionAvail().x;

        ImGui::Dummy(ImVec2(sizeX - 120 - ImGui::GetStyle().WindowPadding.x - ImGui::GetStyle().WindowPadding.x, 0));
        ImGui::SameLine();
        if (ImGui::Button("OK", ImVec2(60, 0)))
        {
            Options::Get().Save();
            visible = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(60, 0)))
        {
            Options::Get().Load();
            visible = false;
        }

        ImGui::End();
    }
}
