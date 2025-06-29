//
// Created by droc101 on 6/29/25.
//

#include "OptionsWindow.h"
#include <cstring>
#include <SDL3/SDL_dialog.h>
#include "imgui.h"
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
    if (filelist == nullptr || filelist[0] == nullptr) return;
    strncpy(Options::gamePath.data(), filelist[0], Options::gamePath.size());
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
        ImGui::PushItemWidth(-40);
        ImGui::InputText("##gamepathinput", Options::gamePath.data(), Options::gamePath.size());
        ImGui::SameLine();
        if (ImGui::Button("..."))
        {
            SDL_ShowOpenFolderDialog(gamePathCallback, nullptr, window, nullptr, false);
        }

        ImGui::Dummy(ImVec2(0, 16));
        if (ImGui::Button("OK", ImVec2(60, 0))) visible = false;

        ImGui::End();
    }
}
