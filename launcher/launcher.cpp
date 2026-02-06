//
// Created by droc101 on 11/10/25.
//
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/Options.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/windows/SetupWindow.h>
#include <imgui.h>
#include <libassets/util/Error.h>
#include <nlohmann/json.hpp>
#include <SDL3/SDL_filesystem.h>
#include <sstream>
#include <string>
#include <vector>

#include "game_sdk/SharedMgr.h"

static std::string sdkPath;
static nlohmann::ordered_json launcher_json;

static std::string selectionCategory;
static std::string selectionIndex;

static Error::ErrorCode LoadLauncherConfig()
{
    std::ifstream file("assets/launcher/launcher.json");
    if (!file.is_open())
    {
        return Error::ErrorCode::CANT_OPEN_FILE;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string j = ss.str();
    launcher_json = nlohmann::ordered_json::parse(j);
    if (launcher_json.is_discarded())
    {
        file.close();
        // printf("File %s is not valid JSON\n", path.c_str());
        return Error::ErrorCode::INCORRECT_FORMAT;
    }

    selectionCategory = launcher_json.at("categories").items().begin().key();
    selectionIndex = launcher_json.at("categories").items().begin().value().items().begin().key();

    return Error::ErrorCode::OK;
}

static constexpr void StringReplace(std::string &string, const std::string &find, const std::string &replace)
{
    std::size_t pos = string.find(find, 0);
    while (pos != std::string::npos)
    {
        string.replace(pos, find.size(), replace);
        pos = string.find(find, pos + find.size());
    }
}

static void ParsePath(std::string &path)
{
    StringReplace(path, "/", "\\");
    StringReplace(path, "$GAMEDIR", Options::Get().gamePath);
    StringReplace(path, "$ASSETSDIR", Options::Get().GetAssetsPath());
    StringReplace(path, "$SDKDIR", sdkPath);
}

static void LaunchSelectedTool()
{
    const nlohmann::json item = launcher_json.at("categories").at(selectionCategory).at(selectionIndex);
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
        printf("Launching process \"%s\"...\n", folder.c_str());
        if (!DesktopInterface::Get().ExecuteProcessNonBlocking(folder, args))
        {
            SDKWindow::Get().ErrorMessage(std::format("Failed to launch process: {}", SDL_GetError()));
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

static void Render()
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("GAME SDK", nullptr, windowFlags);
    ImGui::PopStyleVar();

    ImVec2 wndArea = ImGui::GetContentRegionAvail();

    if (ImGui::BeginChild("##list", ImVec2(wndArea.x, wndArea.y - 36), ImGuiChildFlags_Borders))
    {
        for (const auto &[category, items]: launcher_json.at("categories").items())
        {
            ImGui::SeparatorText(category.c_str());
            for (const auto &[key, value]: items.items())
            {
                ImTextureID textureId = 0;
                (void)SharedMgr::Get().textureCache.GetTextureID(value.value("icon", "file"), textureId);
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
        SetupWindow::Get().Show(false);
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

    ImGui::End();
}

int main()
{
    if (!SDKWindow::Get().Init("GAME SDK", {350, 400}, 0))
    {
        return -1;
    }

    sdkPath = SDL_GetBasePath();
    sdkPath.pop_back();

    const std::vector<std::string> icons = SharedMgr::Get().ScanFolder("assets/launcher/icons", ".png", true);
    for (const std::string &icon: icons)
    {
        const size_t dotIndex = icon.find_last_of('.');
        const std::string basename = icon.substr(0, dotIndex);
        (void)SharedMgr::Get().textureCache.RegisterPng("assets/launcher/icons/" + icon, basename);
    }

    const Error::ErrorCode c = LoadLauncherConfig();
    if (c != Error::ErrorCode::OK)
    {
        printf("Failed to load launcher.json: %s\n", Error::ErrorString(c).c_str());
        return -1;
    }

    SDKWindow::Get().MainLoop(Render);

    SDKWindow::Get().Destroy();

    return 0;
}
