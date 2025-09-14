//
// Created by droc101 on 7/1/25.
//

#include "SharedMgr.h"
#include <cstdio>
#include <filesystem>
#include <imgui.h>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <vector>
#include "AboutWindow.h"
#include "Options.h"
#include "OptionsWindow.h"
#include "TextureBrowserWindow.h"

void SharedMgr::DestroySharedMgr()
{
    Options::Save();
}

void SharedMgr::SharedMenuUI(const std::string &programName)
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
        if (ImGui::MenuItem("Dear ImGui Metrics"))
        {
            metricsVisible = true;
        }
        if (ImGui::MenuItem("Dear ImGui Demo"))
        {
            demoVisible = true;
        }
        ImGui::EndMenu();
    }
#endif
    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem(("Wiki page for " + programName).c_str()))
        {
            (void)SDL_OpenURL(("https://wiki.droc101.dev/index.php/" + programName).c_str());
        }
        if (ImGui::MenuItem("Wiki page for GAME SDK"))
        {
            (void)SDL_OpenURL("https://wiki.droc101.dev/index.php/GAME_SDK");
        }
        if (ImGui::MenuItem("Source Code"))
        {
            (void)SDL_OpenURL("https://github.com/droc101/game-sdk");
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
    if (metricsVisible)
    {
        ImGui::ShowMetricsWindow(&metricsVisible);
    }
    if (demoVisible)
    {
        ImGui::ShowDemoWindow(&demoVisible);
    }
}

std::vector<std::string> SharedMgr::ScanFolder(const std::string &directoryPath,
                                               const std::string &extension,
                                               const bool isRoot)
{
    std::vector<std::string> files;
    try
    {
        for (const std::filesystem::directory_entry &entry: std::filesystem::directory_iterator(directoryPath))
        {
            if (entry.is_regular_file())
            {
                if (entry.path().extension() == extension)
                {
                    files.push_back(entry.path().string());
                }
            } else if (entry.is_directory())
            {
                const std::vector<std::string> subfolderFiles = ScanFolder(entry.path().string(), extension, false);
                files.insert(files.end(), subfolderFiles.begin(), subfolderFiles.end());
            }
        }
    } catch (const std::filesystem::filesystem_error &exception)
    {
        printf("std::filesystem_error: %s", exception.what());
    }
    if (isRoot)
    {
        for (std::string &file: files)
        {
            file = file.substr(directoryPath.length() + 1);
        }
    }
    return files;
}

void SharedMgr::ApplyTheme()
{
    if (Options::theme == Options::Theme::SYSTEM)
    {
        if (SDL_GetSystemTheme() == SDL_SYSTEM_THEME_DARK)
        {
            ImGui::StyleColorsDark();
        } else
        {
            ImGui::StyleColorsLight();
        }
    } else if (Options::theme == Options::Theme::LIGHT)
    {
        ImGui::StyleColorsLight();
    } else
    {
        ImGui::StyleColorsDark();
    }
}

