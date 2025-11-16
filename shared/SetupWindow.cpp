//
// Created by droc101 on 11/12/25.
//

#include "SetupWindow.h"
#include <cstdlib>
#include <imgui.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_video.h>
#include <misc/cpp/imgui_stdlib.h>

#include "Options.h"

void SetupWindow::Show()
{
    visible = true;
}

void SetupWindow::gamePathCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/)
{
    if (filelist == nullptr || filelist[0] == nullptr)
    {
        return;
    }
    Options::gamePath = filelist[0];
    Options::Save();
}

void SetupWindow::Render(SDL_Window *window)
{
    if (!visible) return;
    ImGui::OpenPopup("Setup");
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoResize;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (!ImGui::BeginPopupModal("Setup", nullptr, windowFlags)) return;
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
        ImGui::TextColored(ImVec4(1,0,0,1), "Invalid game path");
    } else
    {
        ImGui::Text(" ");
    }



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
    if (ImGui::Button("Quit", ImVec2(60, 0)))
    {
        Options::Load();
        exit(1);
    }



    ImGui::EndPopup();
}
