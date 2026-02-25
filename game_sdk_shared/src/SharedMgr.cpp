//
// Created by droc101 on 7/1/25.
//

#include <algorithm>
#include <cstdio>
#include <filesystem>
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
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/OptionDefinition.h>
#include <libassets/type/paramDefs/OptionParamDefinition.h>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <libassets/util/Error.h>
#include <memory>
#include <ranges>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_misc.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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
    LoadOptionDefinitions();
    LoadActorDefinitions();
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
        printf("std::filesystem_error: %s\n", exception.what());
    }
    if (isRoot)
    {
        for (std::string &file: files)
        {
            file = file.substr(directoryPath.length() + 1);
        }
        std::ranges::sort(files, [](const std::string &a, const std::string &b) {
            return std::filesystem::path(a).filename().string() < std::filesystem::path(b).filename().string();
        });
    }
    return files;
}

void SharedMgr::LoadOptionDefinitions()
{
    const std::vector<std::string> defs = SharedMgr::ScanFolder(Options::Get().GetAssetsPath() + "/defs/options",
                                                                ".json",
                                                                true);
    for (const std::string &path: defs)
    {
        OptionDefinition def{};
        std::string fullPath = Options::Get().GetAssetsPath() + "/defs/options/" + path;
        const Error::ErrorCode e = OptionDefinition::Create(fullPath, def);
        if (e == Error::ErrorCode::OK)
        {
            optionDefinitions[def.GetName()] = def;
        } else
        {
            printf("Failed to load option def %s: %s\n", fullPath.c_str(), Error::ErrorString(e).c_str());
        }
    }
    printf("Loaded %zu option definitions\n", optionDefinitions.size());
}

void SharedMgr::LoadActorDefinitions()
{
    const std::vector<std::string> defs = SharedMgr::ScanFolder(Options::Get().GetAssetsPath() + "/defs/actors",
                                                                ".json",
                                                                true);
    for (const std::string &path: defs)
    {
        ActorDefinition def{};
        const std::string fullPath = Options::Get().GetAssetsPath() + "/defs/actors/" + path;
        const Error::ErrorCode e = ActorDefinition::Create(fullPath, def);
        if (e == Error::ErrorCode::OK)
        {
            actorDefinitions[def.className] = def;
        } else
        {
            printf("Failed to load actor def %s: %s\n", fullPath.c_str(), Error::ErrorString(e).c_str());
        }
    }

    for (std::pair<const std::string, ActorDefinition> &def: actorDefinitions)
    {
        if (!def.second.parentClassName.empty())
        {
            if (def.second.parentClassName == def.first)
            {
                throw std::runtime_error("An actor cannot be its own parent");
            }
            def.second.parentClass = &actorDefinitions.at(def.second.parentClassName);
        }

        for (const std::shared_ptr<ParamDefinition> &val: def.second.params | std::views::values)
        {
            OptionParamDefinition *opt = dynamic_cast<OptionParamDefinition *>(val.get());
            if (opt != nullptr)
            {
                opt->definition = &optionDefinitions.at(opt->optionListName);
                if (opt->definition == nullptr)
                {
                    throw std::runtime_error("Failed to find option definition!");
                }
            }
        }
    }

    printf("Loaded %zu actor definitions\n", actorDefinitions.size());

    if (!actorDefinitions.contains("player"))
    {
        printf("WARNING: No \"player\" actor class definition was loaded!\n");
    }
    if (!actorDefinitions.contains("actor"))
    {
        printf("WARNING: No \"actor\" actor class definition was loaded!\n");
    }
}
