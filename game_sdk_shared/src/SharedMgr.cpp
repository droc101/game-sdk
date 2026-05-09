//
// Created by droc101 on 7/1/25.
//

#include <game_sdk/DesktopInterface.h>
#include <game_sdk/Options.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/windows/AboutWindow.h>
#include <game_sdk/windows/MaterialBrowserWindow.h>
#include <game_sdk/windows/ModelBrowserWindow.h>
#include <game_sdk/windows/OptionsWindow.h>
#include <game_sdk/windows/SetupWindow.h>
#include <game_sdk/windows/TextureBrowserWindow.h>
#include <imgui.h>
#include <libassets/asset/DataAsset.h>
#include <libassets/util/Error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_misc.h>
#include <string>

#ifdef WIN32
#include <direct.h> // provides chdir
#else
#include <unistd.h>
#endif

SharedMgr &SharedMgr::Get()
{
    static SharedMgr sharedMgrSingleton{};

    return sharedMgrSingleton;
}

void SharedMgr::InitSharedMgr()
{
    chdir(SDL_GetBasePath());
    Options::Get().Load();
    UpdateAssetPaths();
    DesktopInterface::Get().InitDesktopInterface();
    if (!Options::Get().ValidateGamePath())
    {
        SetupWindow::Get().Show();
    }
}

void SharedMgr::DestroySharedMgr()
{
    Options::Get().Save();
}

void SharedMgr::SharedMenuUI(const std::string &programName)
{
    if (ImGui::BeginMenu("Tools"))
    {
        if (ImGui::MenuItem("Options"))
        {
            OptionsWindow::Get().Show();
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
        if (!programName.empty())
        {
            if (ImGui::MenuItem(("Wiki page for " + programName).c_str()))
            {
                (void)SDL_OpenURL(("https://wiki.droc101.dev/index.php/" + programName).c_str());
            }
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
            AboutWindow::Get().Show();
        }
        ImGui::EndMenu();
    }
}

void SharedMgr::RenderSharedUI()
{
    OptionsWindow::Get().Render();
    AboutWindow::Get().Render();
    TextureBrowserWindow::Get().Render();
    MaterialBrowserWindow::Get().Render();
    ModelBrowserWindow::Get().Render();
    SetupWindow::Get().Render();
    if (metricsVisible)
    {
        ImGui::ShowMetricsWindow(&metricsVisible);
    }
    if (demoVisible)
    {
        ImGui::ShowDemoWindow(&demoVisible);
    }
}

void SharedMgr::UpdateAssetPaths()
{
    DataAsset gameConfig{};
    const Error::ErrorCode e = DataAsset::CreateFromAsset(Options::Get().gameConfigPath.c_str(), gameConfig);
    if (e != Error::ErrorCode::OK)
    {
        return;
    }
    pathManager = SearchPathManager(gameConfig,
                                    Options::Get().GetExecutablePath(),
                                    Options::Get().GetGameConfigParentPath());
}
