//
// Created by droc101 on 11/12/25.
//

#include <array>
#include <cstdlib>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/Options.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/windows/SetupWindow.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string>
#include <cmath>

SetupWindow &SetupWindow::Get()
{
    static SetupWindow setupWindowSingleton{};
    return setupWindowSingleton;
}

void SetupWindow::Show(bool required)
{
    visible = true;
    SetupWindow::required = required;
}

void SetupWindow::gamePathCallback(const std::string &path)
{
    Options::Get().gameExecutablePath = path;
}

void SetupWindow::assetsPathCallback(const std::string &path)
{
    Options::Get().gameConfigPath = path;
}

void SetupWindow::Render()
{
    if (!visible)
    {
        return;
    }
    ImGui::OpenPopup("Setup");
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    const float width = fminf(450, viewport->WorkSize.x - 40);
    ImGui::SetNextWindowSize({width, -1});
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoResize |
                                             ImGuiWindowFlags_NoDocking;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (!ImGui::BeginPopupModal("Setup", nullptr, windowFlags))
    {
        return;
    }
    ImGui::PopStyleVar();

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
    ImGui::BeginDisabled(!valid);
    if (ImGui::Button("OK", ImVec2(60, 0)))
    {
        Options::Get().Save();
        visible = false;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (required)
    {
        if (ImGui::Button("Quit", ImVec2(60, 0)))
        {
            Options::Get().Load();
            exit(1);
        }
    } else
    {
        if (ImGui::Button("Cancel", ImVec2(60, 0)))
        {
            Options::Get().Load();
            visible = false;
        }
    }

    ImGui::EndPopup();
}
