#include <cstdio>
#include <format>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <libassets/asset/LevelAsset.h>
#include <libassets/util/Error.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include "DialogFilters.h"
#include "SDLRendererImGuiTextureAssetCache.h"
#include "SharedMgr.h"

static LevelAsset level{};
static bool levelLoaded = false;
static SDL_Renderer *renderer = nullptr;
static SDL_Window *window = nullptr;

static void destroyExistingLevel()
{
    if (!levelLoaded)
    {
        return;
    }
    levelLoaded = false;
}

static void openGmapCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    destroyExistingLevel();
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = LevelAsset::CreateFromAsset(fileList[0], level);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to open the level!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
        return;
    }
    levelLoaded = true;
}

static void importCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    destroyExistingLevel();
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = LevelAsset::CreateFromBin(fileList[0], level);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to import the level!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
        return;
    }
    levelLoaded = true;
}

static void saveGmapCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = level.SaveAsAsset(fileList[0]);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to save the level!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
    }
}

static void exportCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = level.SaveAsBin(fileList[0]);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to export the level!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
    }
}

static void Render(bool &done, SDL_Window *sdlWindow)
{
    ImGui::Begin("lvledit",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    bool importPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_O);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && levelLoaded;
    bool exportPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S) && levelLoaded;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            importPressed |= ImGui::MenuItem("Import", "Ctrl+Shift+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S", false, levelLoaded);
            exportPressed |= ImGui::MenuItem("Export", "Ctrl+Shift+S", false, levelLoaded);
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                done = true;
            }
            ImGui::EndMenu();
        }
        SharedMgr::SharedMenuUI("lvledit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDL_ShowOpenFileDialog(openGmapCallback,
                               nullptr,
                               sdlWindow,
                               DialogFilters::gmapFilters.data(),
                               1,
                               nullptr,
                               false);
    } else if (importPressed)
    {
        SDL_ShowOpenFileDialog(importCallback, nullptr, sdlWindow, DialogFilters::binFilters.data(), 1, nullptr, false);
    } else if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveGmapCallback, nullptr, sdlWindow, DialogFilters::gmapFilters.data(), 1, nullptr);
    } else if (exportPressed)
    {
        SDL_ShowSaveFileDialog(exportCallback, nullptr, sdlWindow, DialogFilters::binFilters.data(), 1, nullptr);
    }

    if (levelLoaded)
    {
        ImGui::TextUnformatted(std::format("Level Loaded\nSize: {} bytes", level.GetDataSize()).c_str());
        ImGui::TextDisabled("You currently still have to use");
        ImGui::SameLine();
        ImGui::TextLinkOpenURL("geditor", "https://github.com/droc101/game-editor");
        ImGui::SameLine();
        ImGui::TextDisabled("to actually edit the levels.");
        ImGui::TextDisabled("This tool is only for compiling and decompiling.");

    } else
    {
        ImGui::TextDisabled("No level is open. Open or import one from the File menu.");
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

    constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow("lvledit", 800, 600, windowFlags);
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
    if (!SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED))
    {
        printf("Error: SDL_SetWindowPosition(): %s\n", SDL_GetError());
    }

    SharedMgr::InitSharedMgr<SDLRendererImGuiTextureAssetCache>(renderer);

    if (!SDL_ShowWindow(window))
    {
        printf("Error: SDL_ShowWindow(): %s\n", SDL_GetError());
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (SDL_GetSystemTheme() == SDL_SYSTEM_THEME_DARK)
    {
        ImGui::StyleColorsDark();
    } else
    {
        ImGui::StyleColorsLight();
    }

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
    destroyExistingLevel();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
