#include <array>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <iostream>
#include <SDL3/SDL.h>
#include "Options.h"
#include "SharedMgr.h"
#include <libassets/asset/LevelAsset.h>
#include "SDLRendererImGuiTextureAssetCache.h"

static LevelAsset level;
static bool levelLoaded = false;
SDL_Renderer *renderer;

constexpr SDL_DialogFileFilter gmapFilter = {"Compiled GAME map (*.gmap)", "gmap"};
constexpr SDL_DialogFileFilter binFilter = {"Raw GAME map (*.bin)", "bin"};

void destroyExistingLevel()
{
    if (!levelLoaded)
    {
        return;
    }
    levelLoaded = false;
}

void openGtexCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    destroyExistingLevel();
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode e = LevelAsset::CreateFromAsset(fileList[0], level);
    assert(e == Error::ErrorCode::E_OK);
    levelLoaded = true;
}

void importCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    destroyExistingLevel();
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode e = LevelAsset::CreateFromBin(fileList[0], level);
    assert(e == Error::ErrorCode::E_OK);
    levelLoaded = true;
}

void saveGtexCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    assert(level.SaveAsAsset(fileList[0]) == Error::ErrorCode::E_OK);
}

void exportCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    assert(level.SaveAsBin(fileList[0]) == Error::ErrorCode::E_OK);
}

static void Render(bool &done, SDL_Window *window)
{
    ImGui::Begin("lvledit",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
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
        SharedMgr::SharedMenuUI();
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDL_ShowOpenFileDialog(openGtexCallback, nullptr, window, {&gmapFilter}, 1, nullptr, false);
    } else if (importPressed)
    {
        SDL_ShowOpenFileDialog(importCallback, nullptr, window, {&binFilter}, 1, nullptr, false);
    } else if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveGtexCallback, nullptr, window, {&gmapFilter}, 1, nullptr);
    } else if (exportPressed)
    {
        SDL_ShowSaveFileDialog(exportCallback, nullptr, window, {&binFilter}, 1, nullptr);
    }

    if (levelLoaded)
    {
        ImGui::Text("Level Loaded\nSize: %ld bytes", level.GetDataSize());
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
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
    SDL_Window *window = SDL_CreateWindow("lvledit", 800, 600, windowFlags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    if (renderer == nullptr)
    {
        printf("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    SDLRendererImGuiTextureAssetCache cache = SDLRendererImGuiTextureAssetCache(renderer);
    SharedMgr::InitSharedMgr(&cache);

    SDL_ShowWindow(window);

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
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 1);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
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
