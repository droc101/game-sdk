//
// Created by droc101 on 7/1/25.
//

#include "SharedMgr.h"
#include <filesystem>
#include <vector>
#include <SDL3/SDL_misc.h>
#include "AboutWindow.h"
#include "imgui.h"
#include "Options.h"
#include "OptionsWindow.h"
#include "TextureBrowserWindow.h"

void SharedMgr::InitSharedMgr(ImGuiTextureAssetCache *cache)
{
    Options::Load();
    textureCache = cache;
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
#ifdef BUILDSTYLE_DEBUG
    if (ImGui::BeginMenu("Debug"))
    {
        if (ImGui::MenuItem("Dear ImGui Metrics")) metricsVisible = true;
        if (ImGui::MenuItem("Dear ImGui Demo")) demoVisible = true;
        ImGui::EndMenu();
    }
#endif
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

void SharedMgr::RenderSharedUI(SDL_Window *window)
{
    OptionsWindow::Render(window);
    AboutWindow::Render();
    TextureBrowserWindow::Render();
    if (metricsVisible) ImGui::ShowMetricsWindow(&metricsVisible);
    if (demoVisible) ImGui::ShowDemoWindow(&demoVisible);
}

std::vector<std::string> SharedMgr::ScanFolder(const std::string &directory_path, const std::string &extension)
{
    std::vector<std::string> files;
    try
    {
        for (const auto &entry: std::filesystem::directory_iterator(directory_path))
        {
            if (entry.is_regular_file())
            {
                if (entry.path().extension() == extension)
                {
                    files.push_back(entry.path().filename().string());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error &ex)
    {
        printf("std::filesystem_error: %s", ex.what());
    }
    return files;
}
