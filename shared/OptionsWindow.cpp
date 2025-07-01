//
// Created by droc101 on 6/29/25.
//

#include "OptionsWindow.h"
#include <cstring>
#include <imgui.h>
#include <SDL3/SDL_dialog.h>
#include "Options.h"

void OptionsWindow::Show()
{
    visible = true;
}

void OptionsWindow::Hide()
{
    visible = false;
}

void OptionsWindow::gamePathCallback(void *, const char *const *filelist, int)
{
    if (filelist == nullptr || filelist[0] == nullptr)
    {
        return;
    }
    std::strncpy(Options::gamePath.data(), filelist[0], Options::gamePath.size() - 1);
    Options::Save();
}

void OptionsWindow::Render(SDL_Window *window)
{
    if (visible)
    {
        ImGui::SetNextWindowSize(ImVec2(400, -1));
        ImGui::Begin("GAME SDK Options",
                     &visible,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

        ImGui::Text("Folder with GAME executable");
        ImGui::SameLine();
        ImGui::TextDisabled("(no trailing slash)");
        ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x - 40);
        ImGui::InputText("##gamepathinput", Options::gamePath.data(), Options::gamePath.size());
        ImGui::SameLine();
        if (ImGui::Button("...", ImVec2(40, 0)))
        {
            SDL_ShowOpenFolderDialog(gamePathCallback, nullptr, window, nullptr, false);
        }

        ImGui::Dummy(ImVec2(0, 16));

        const float sizeX = ImGui::GetContentRegionAvail().x;

        ImGui::Dummy(ImVec2(sizeX - 120 - ImGui::GetStyle().WindowPadding.x - ImGui::GetStyle().WindowPadding.x, 0));
        ImGui::SameLine();
        if (ImGui::Button("OK", ImVec2(60, 0)))
        {
            visible = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(60, 0)))
        {
            Options::Load();
            visible = false;
        }

        ImGui::End();
    }
}
