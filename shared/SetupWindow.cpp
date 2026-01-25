//
// Created by droc101 on 11/12/25.
//

#include "SetupWindow.h"
#include <cstdlib>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_video.h>

#include "Options.h"

void SetupWindow::Show(bool required)
{
    visible = true;
    SetupWindow::required = required;
}

void SetupWindow::gamePathCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/)
{
    if (filelist == nullptr || filelist[0] == nullptr)
    {
        return;
    }
    Options::gamePath = filelist[0];
}

void SetupWindow::assetsPathCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/)
{
    if (filelist == nullptr || filelist[0] == nullptr)
    {
        return;
    }
    Options::assetsPath = filelist[0];
}

void SetupWindow::Render(SDL_Window *window)
{
    if (!visible)
    {
        return;
    }
    ImGui::OpenPopup("Setup");
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
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

    ImGui::TextUnformatted("Folder with GAME executable");
    ImGui::SameLine();
    ImGui::TextDisabled("(no trailing slash)");
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::InputText("##gamepathinput", &Options::gamePath);
    ImGui::SameLine();
    if (ImGui::Button("...", ImVec2(40, 0)))
    {
        SDL_ShowOpenFolderDialog(gamePathCallback, nullptr, window, nullptr, false);
    }
    bool valid = true;
    if (!Options::ValidateGamePath())
    {
        valid = false;
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid game path");
    } else
    {
        ImGui::Text(" ");
    }

    ImGui::Checkbox("Override Assets Directory", &Options::overrideAssetsPath);
    ImGui::SameLine();
    ImGui::TextDisabled("(no trailing slash)");
    ImGui::BeginDisabled(!Options::overrideAssetsPath);
    ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
    ImGui::InputText("##assetspathinput", &Options::assetsPath);
    ImGui::SameLine();
    if (ImGui::Button("...##assets", ImVec2(40, 0)))
    {
        SDL_ShowOpenFolderDialog(assetsPathCallback, nullptr, window, nullptr, false);
    }
    ImGui::EndDisabled();


    ImGui::Dummy(ImVec2(0, 16));
    const float sizeX = ImGui::GetContentRegionAvail().x;

    ImGui::Dummy(ImVec2(sizeX - 120 - ImGui::GetStyle().WindowPadding.x - ImGui::GetStyle().WindowPadding.x, 0));
    ImGui::SameLine();
    ImGui::BeginDisabled(!valid);
    if (ImGui::Button("OK", ImVec2(60, 0)))
    {
        Options::Save();
        visible = false;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (required)
    {
        if (ImGui::Button("Quit", ImVec2(60, 0)))
        {
            Options::Load();
            exit(1);
        }
    } else
    {
        if (ImGui::Button("Cancel", ImVec2(60, 0)))
        {
            Options::Load();
            visible = false;
        }
    }

    ImGui::EndPopup();
}
