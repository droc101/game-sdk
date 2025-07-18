#include <array>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <iostream>
#include <unordered_map>
#include <SDL3/SDL.h>
#include "Options.h"
#include <libassets/asset/TextureAsset.h>
#include "SharedMgr.h"
#include <libassets/asset/LevelAsset.h>

static LevelAsset level;
static bool levelLoaded = false;
SDL_Renderer *renderer;
std::unordered_map<std::string, SDL_Texture*> textureBuffers{};

constexpr SDL_DialogFileFilter gmapFilter = {"Compiled GAME map (*.gmap)", "gmap"};
constexpr SDL_DialogFileFilter binFilter = {"Raw GAME map (*.bin)", "bin"};

ImTextureID GetTextureID(const std::string& relPath)
{
    if (textureBuffers.contains(relPath))
    {
        return reinterpret_cast<ImTextureID>(textureBuffers.at(relPath));
    }
    // TODO: if the texture file does not exist return an existing missing texture instead of making a new one
    const std::string &texturePath = Options::gamePath + std::string("/assets/") + relPath;

    const TextureAsset asset = TextureAsset::CreateFromAsset(texturePath.c_str());
    SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(asset.GetWidth()),
                                                 static_cast<int>(asset.GetHeight()),
                                                 SDL_PIXELFORMAT_RGBA8888,
                                                 const_cast<uint32_t *>(asset.GetPixels()),
                                                 static_cast<int>(asset.GetWidth() * sizeof(uint32_t)));
    if (surface == nullptr)
    {
        printf("SDL_CreateSurfaceFrom() failed: %s\n", SDL_GetError());
        return -1;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
    if (tex == nullptr)
    {
        printf("SDL_CreateTextureFromSurface() failed: %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(surface);

    textureBuffers.insert({relPath, tex});
    return reinterpret_cast<ImTextureID>(tex);
}

ImVec2 GetTextureSize(const std::string &relPath)
{
    SDL_Texture* tex = reinterpret_cast<SDL_Texture *>(GetTextureID(relPath));
    ImVec2 sz;
    SDL_GetTextureSize(tex, &sz.x, &sz.y);
    return sz;
}

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
    level = LevelAsset::CreateFromAsset(fileList[0]);
    levelLoaded = true;
}

void importCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    destroyExistingLevel();
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    level = LevelAsset::CreateFromBin(fileList[0]);
    levelLoaded = true;
}

void saveGtexCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    level.SaveAsAsset(fileList[0]);
}

void exportCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    level.SaveAsBin(fileList[0]);
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

    SharedMgr::InitSharedMgr();
    SharedMgr::GetTextureId = GetTextureID;
    SharedMgr::GetTextureSize = GetTextureSize;

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
    for (const std::pair<std::string, SDL_Texture*> p: textureBuffers)
    {
        SDL_DestroyTexture(p.second);
    }
    textureBuffers.clear();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
