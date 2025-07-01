//
// Created by droc101 on 7/1/25.
//

#include "SharedMgr.h"
#include <SDL3/SDL_misc.h>
#include "AboutWindow.h"
#include "imgui.h"
#include "Options.h"
#include "OptionsWindow.h"

void SharedMgr::InitSharedMgr()
{
    Options::Load();
}

void SharedMgr::DestroySharedMgr()
{
    Options::Save();
}

void SharedMgr::SharedMenuUI()
{
    if (ImGui::BeginMenu("Tools"))
    {
        if (ImGui::MenuItem("Options"))
        {
            OptionsWindow::Show();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem("Source Code"))
        {
            SDL_OpenURL("https://github.com/droc101/game-sdk");
        }
        if (ImGui::MenuItem("About"))
        {
            AboutWindow::Show();
        }
        ImGui::EndMenu();
    }
}

void SharedMgr::RenderSharedUI(SDL_Window* window)
{
    OptionsWindow::Render(window);
    AboutWindow::Render();
}
