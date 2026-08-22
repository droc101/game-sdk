//
// Created by droc101 on 8/20/26.
//

#include "LauncherWindow.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/Options.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/Window.h>
#include <game_sdk/WindowManager.h>
#include <game_sdk/windows/SetupWindow.h>
#include <imgui.h>
#include <libassets/util/Error.h>
#include <libassets/util/FileIo.h>
#include <libassets/util/Logger.h>
#include <libassets/util/SearchPathManager.h>
#include <memory>
#include <SDL3/SDL_filesystem.h>
#include <vector>

bool LauncherWindow::Init()
{
    sdkPath = SDL_GetBasePath();
    sdkPath.pop_back();

    const std::vector<std::string> icons = SearchPathManager::ScanFolder("assets/icons", ".png", true);
    for (const std::string &icon: icons)
    {
        const size_t dotIndex = icon.find_last_of('.');
        const std::string basename = icon.substr(0, dotIndex);
        (void)SharedMgr::Get().textureCache.RegisterPng("assets/icons/" + icon, basename);
    }

    const Error::ErrorCode c = LoadLauncherConfig();
    if (c != Error::ErrorCode::OK)
    {
        Logger::Error("Failed to load launcher.json: %s", Error::ErrorString(c).c_str());
        return false;
    }

    return true;
}

void LauncherWindow::Render()
{
    ImVec2 wndArea = ImGui::GetContentRegionAvail();

    if (ImGui::BeginChild("##list", ImVec2(wndArea.x, wndArea.y - 36), ImGuiChildFlags_Borders))
    {
        for (const auto &[category, items]: launcherJson.at("categories").items())
        {
            ImGui::SeparatorText(category.c_str());
            for (const auto &[key, value]: items.items())
            {
                ImTextureID textureId = 0;
                if (SharedMgr::Get().textureCache.GetTextureID(value.value("icon", "file"), textureId) !=
                    Error::ErrorCode::OK)
                {
                    textureId = SharedMgr::Get().textureCache.GetMissingTextureID();
                }
                const std::string title = std::format("##item_{}_{}", category, key);
                const bool selected = ImGui::Selectable(title.c_str(),
                                                        selectionCategory == category && selectionIndex == key,
                                                        ImGuiSelectableFlags_AllowOverlap |
                                                                ImGuiSelectableFlags_SpanAllColumns,
                                                        {0, 18});
                if (selected)
                {
                    selectionCategory = category;
                    selectionIndex = key;
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    LaunchSelectedTool();
                }
                ImGui::SameLine();
                ImGui::Image(textureId, {18, 18});
                ImGui::SameLine();
                ImGui::Text("%s", key.c_str());
            }
        }
        ImGui::EndChild();
    }

    if (ImGui::Button("Options", ImVec2(80, 32)))
    {
        WindowManager::Get().AddWindow(std::make_shared<SetupWindow>(false));
    }
    // ImGui::SameLine();
    // ImGui::TextDisabled("GAME SDK\nVersion %s", LIBASSETS_VERSION_STRING);
    ImGui::SameLine();
    wndArea = ImGui::GetContentRegionAvail();
    ImGui::Dummy(ImVec2(wndArea.x - 80 - 8, 1));
    ImGui::SameLine();
    if (ImGui::Button("Launch", ImVec2(80, 32)))
    {
        LaunchSelectedTool();
    }
}

const Window::WindowProperties &LauncherWindow::GetProperties() const
{
    return properties;
}

Error::ErrorCode LauncherWindow::LoadLauncherConfig()
{
    std::string jsonStr;
    const Error::ErrorCode readError = FileIo::ReadFileToString("assets/launcher.json", jsonStr);
    if (readError != Error::ErrorCode::OK)
    {
        return readError;
    }
    launcherJson = nlohmann::ordered_json::parse(jsonStr);
    if (launcherJson.is_discarded())
    {
        // printf("File %s is not valid JSON\n", path.c_str());
        return Error::ErrorCode::INCORRECT_FORMAT;
    }

    selectionCategory = launcherJson.at("categories").items().begin().key();
    selectionIndex = launcherJson.at("categories").items().begin().value().items().begin().key();

    return Error::ErrorCode::OK;
}

constexpr void LauncherWindow::StringReplace(std::string &string, const std::string &find, const std::string &replace)
{
    std::size_t pos = string.find(find, 0);
    while (pos != std::string::npos)
    {
        string.replace(pos, find.size(), replace);
        pos = string.find(find, pos + find.size());
    }
}

void LauncherWindow::ParsePath(std::string &path)
{
#ifdef WIN32
    StringReplace(path, "/", "\\");
#endif
    StringReplace(path, "$GAMEDIR", Options::Get().GetExecutablePath());
    StringReplace(path, "$ASSETSDIR", Options::Get().GetAssetsPath());
    StringReplace(path, "$SDKDIR", sdkPath);
}

void LauncherWindow::LaunchSelectedTool()
{
    const nlohmann::json item = launcherJson.at("categories").at(selectionCategory).at(selectionIndex);
    if (item.contains("binary"))
    {
        std::string workdir = item.value("workdir", "$SDKDIR");
        ParsePath(workdir);
        if (std::filesystem::is_directory(workdir))
        {
            std::filesystem::current_path(workdir);
        }

        std::string folder = item.value("binary", "");
        ParsePath(folder);
#ifdef WIN32
        folder += ".exe";
#endif
        std::vector<std::string> args{};
        for (std::string &arg: item.value("arguments", std::vector<std::string>{}))
        {
            ParsePath(arg);
            args.push_back(arg);
        }
        Logger::Info("Launching process \"{}\"...", folder.c_str());
        if (!DesktopInterface::Get().ExecuteProcessNonBlocking(folder, args))
        {
            ErrorMessage(std::format("Failed to launch process: {}", SDL_GetError()));
        }
    } else if (item.contains("file"))
    {
        std::string folder = item.value("file", "");
        ParsePath(folder);
        DesktopInterface::Get().OpenFilesystemPath(folder);
    } else if (item.contains("folder"))
    {
        std::string folder = item.value("folder", "");
        ParsePath(folder);
        DesktopInterface::Get().OpenFilesystemPath(folder);
    } else if (item.contains("url"))
    {
        DesktopInterface::Get().OpenURL(item.value("url", ""));
    }
}
