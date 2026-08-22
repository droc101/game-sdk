//
// Created by droc101 on 7/1/25.
//

#include <game_sdk/DesktopInterface.h>
#include <game_sdk/Options.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/WindowManager.h>
#include <game_sdk/windows/AboutWindow.h>
#include <game_sdk/windows/MaterialBrowserWindow.h>
#include <game_sdk/windows/ModelBrowserWindow.h>
#include <game_sdk/windows/OptionsWindow.h>
#include <game_sdk/windows/SoundBrowserWindow.h>
#include <game_sdk/windows/TextureBrowserWindow.h>
#include <imgui.h>
#include <libassets/asset/DataAsset.h>
#include <libassets/util/Error.h>
#include <memory>
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
        // SetupWindow::Get().Show(); // TODO: show setup
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
            WindowManager::Get().AddModalWindow(std::make_shared<OptionsWindow>());
        }
        ImGui::EndMenu();
    }
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
            WindowManager::Get().AddModalWindow(std::make_shared<AboutWindow>());
        }
        ImGui::EndMenu();
    }
}

void SharedMgr::UpdateAssetPaths()
{
    DataAsset gameConfig{};
    const Error::ErrorCode e = gameConfig.LoadFromAsset(Options::Get().gameConfigPath);
    if (e != Error::ErrorCode::OK)
    {
        return;
    }
    pathManager = SearchPathManager(gameConfig,
                                    Options::Get().GetExecutablePath(),
                                    Options::Get().GetGameConfigParentPath());
}
