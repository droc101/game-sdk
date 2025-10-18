//
// Created by droc101 on 10/4/25.
//

#include <cstdio>
#include <format>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <libassets/asset/GameConfigAsset.h>
#include <libassets/util/Error.h>
#include <misc/cpp/imgui_stdlib.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <string>
#include "DialogFilters.h"
#include "SDLRendererImGuiTextureAssetCache.h"
#include "SharedMgr.h"

static GameConfigAsset config{};
static bool configLoaded = false;
static SDL_Renderer *renderer = nullptr;
static SDL_Window *window = nullptr;

static void openGameCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = GameConfigAsset::CreateFromAsset(fileList[0], config);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to open the gane configuration!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Failed to show SDL Error messagebox with error \"%s\"\n", SDL_GetError());
        }
        return;
    }
    configLoaded = true;
}

static void saveGameCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = config.SaveAsAsset(fileList[0]);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to save the game configuration!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Failed to show SDL Error messagebox with error \"%s\"\n", SDL_GetError());
        }
    }
}

static void Render(bool &done, SDL_Window *sdlWindow)
{
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("cfgedit", nullptr, windowFlags);
    bool newPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && configLoaded;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            newPressed |= ImGui::MenuItem("New", "Ctrl+N");
            ImGui::Separator();
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S", false, configLoaded);
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                done = true;
            }
            ImGui::EndMenu();
        }
        SharedMgr::SharedMenuUI("cfgedit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDL_ShowOpenFileDialog(openGameCallback,
                               nullptr,
                               sdlWindow,
                               DialogFilters::gameFilters.data(),
                               1,
                               nullptr,
                               false);
    } else if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveGameCallback, nullptr, sdlWindow, DialogFilters::gameFilters.data(), 1, nullptr);
    } else if (newPressed)
    {
        config = GameConfigAsset();
        configLoaded = true;
    }

    if (configLoaded)
    {
        ImGui::PushItemWidth(-1);
        ImGui::Text("Game Title");
        ImGui::InputText("##gameTitle", &config.gameTitle);
        ImGui::Text("Game Copyright");
        ImGui::InputText("##gameCopyright", &config.gameCopyright);

        ImGui::Dummy({0, 12});
        ImGui::Separator();
        ImGui::Dummy({0, 12});

        ImGui::Text("Discord Rich Presence App ID");
        ImGui::SameLine();
        ImGui::TextDisabled("(set to 0 to disable)");
        ImGui::InputScalar("##appId", ImGuiDataType_U64, &config.discordAppId);
    } else
    {
        ImGui::TextDisabled("No game configuration is open.\nOpen or create one from the File menu.");
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

    constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_HIDDEN;
    window = SDL_CreateWindow("cfgedit", 400, 200, windowFlags);
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

    if (!SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED))
    {
        printf("Error: SDL_SetWindowPosition(): %s\n", SDL_GetError());
    }
    if (!SDL_ShowWindow(window))
    {
        printf("Error: SDL_ShowWindow(): %s\n", SDL_GetError());
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

        Render(done, window);

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
