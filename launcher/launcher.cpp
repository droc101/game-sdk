//
// Created by droc101 on 11/10/25.
//
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <libassets/util/Error.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <sstream>
#include <string>
#include "DesktopInterface.h"
#include "Options.h"
#include "SDLRendererImGuiTextureAssetCache.h"
#include "SharedMgr.h"

static SDL_Renderer *renderer = nullptr;
static SDL_Window *window = nullptr;

static std::string sdkPath;
static nlohmann::ordered_json launcher_json;

static std::string selectionCategory;
static std::string selectionIndex;

static Error::ErrorCode LoadLauncherConfig()
{
    std::ifstream file("assets/launcher.json");
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

static void StringReplace(std::string &string, const std::string &find, const std::string &replace)
{
    std::string buffer;
    std::size_t pos = 0;
    std::size_t prevPos = 0;
    buffer.reserve(string.size());

    while (true)
    {
        prevPos = pos;
        pos = string.find(find, pos);
        if (pos == std::string::npos)
        {
            break;
        }
        buffer.append(string, prevPos, pos - prevPos);
        buffer += replace;
        pos += find.size();
    }

    buffer.append(string, prevPos, string.size() - prevPos);
    string.swap(buffer);
}

static void ParsePath(std::string &path)
{
    StringReplace(path, "$GAMEDIR", Options::gamePath);
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
        printf("Launching process \"%s\"...\n", folder.c_str());
#ifdef WIN32
        DesktopInterface::OpenFilesystemPath(folder);
#else
        DesktopInterface::ExecuteProcessNonBlocking(folder, {});
#endif
    } else if (item.contains("file"))
    {
        std::string folder = item.value("file", "");
        ParsePath(folder);
        DesktopInterface::OpenFilesystemPath(folder);
    } else if (item.contains("folder"))
    {
        std::string folder = item.value("folder", "");
        ParsePath(folder);
        DesktopInterface::OpenFilesystemPath(folder);
    } else if (item.contains("url"))
    {
        DesktopInterface::OpenURL(item.value("url", ""));
    }
}

static void Render()
{
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("GAME SDK", nullptr, windowFlags);
    ImGui::PopStyleVar();

    const ImVec2 wndArea = ImGui::GetContentRegionAvail();

    if (ImGui::BeginChild("##list", ImVec2(wndArea.x, wndArea.y - 36), ImGuiChildFlags_Borders))
    {
        for (const auto &[category, items]: launcher_json.at("categories").items())
        {
            ImGui::SeparatorText(category.c_str());
            for (const auto &[key, value]: items.items())
            {
                if (ImGui::Selectable(key.c_str(), selectionCategory == category && selectionIndex == key))
                {
                    selectionCategory = category;
                    selectionIndex = key;
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    LaunchSelectedTool();
                }
            }
        }
        ImGui::EndChild();
    }

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
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow("GAME SDK", 350, 400, 0);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    renderer = SDL_CreateRenderer(window, nullptr);
    if (!SDL_SetRenderVSync(renderer, 1))
    {
        printf("Error: SDL_SetRenderVSync(): %s\n", SDL_GetError());
    }
    if (renderer == nullptr)
    {
        printf("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return -1;
    }

    SharedMgr::InitSharedMgr<SDLRendererImGuiTextureAssetCache>(renderer);
    sdkPath = SDL_GetBasePath();
    sdkPath.pop_back();

    const Error::ErrorCode c = LoadLauncherConfig();
    if (c != Error::ErrorCode::OK)
    {
        printf("Failed to load launcher.json: %s\n", Error::ErrorString(c).c_str());
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    SharedMgr::ApplyTheme();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
            {
                done = true;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
            {
                done = true;
            }
        }

        if ((SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0)
        {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        Render();

        SharedMgr::RenderSharedUI(window);

        ImGui::Render();
        if (!SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y))
        {
            printf("Error: SDL_SetRenderScale(): %s\n", SDL_GetError());
        }
        if (!SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 1))
        {
            printf("Error: SDL_SetRenderDrawColorFloat(): %s\n", SDL_GetError());
        }
        if (!SDL_RenderClear(renderer))
        {
            printf("Error: SDL_RenderClear(): %s\n", SDL_GetError());
        }
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        if (!SDL_RenderPresent(renderer))
        {
            printf("Error: SDL_RenderPresent(): %s\n", SDL_GetError());
        }
    }

    SharedMgr::DestroySharedMgr();

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
