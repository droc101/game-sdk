#include <array>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <iostream>
#include <unordered_map>
#include <SDL3/SDL.h>
#include "Options.h"
#include "libassets/TextureAsset.h"
#include "SharedMgr.h"

static TextureAsset texture;
static bool textureLoaded = false;
SDL_Renderer *renderer;
SDL_Texture *sdlTexture;
SDL_Surface *sdlSurface;
std::unordered_map<std::string, SDL_Texture*> textureBuffers{};

constexpr SDL_DialogFileFilter gtexFilter = {"GAME texture (*.gtex)", "gtex"};
constexpr SDL_DialogFileFilter pngFilter = {"PNG Image", "png"};
constexpr std::array imageFilters = {
        SDL_DialogFileFilter{"Images", "png;jpg;jpeg;tga"},
        SDL_DialogFileFilter{"PNG Images", "png"},
        SDL_DialogFileFilter{"JPG Images", "jpg;jepg"},
        SDL_DialogFileFilter{"TGA Images", "tga"},
};

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
                                                 static_cast<int>(asset.GetWidth() * sizeof(uint)));
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

void destroyExistingTexture()
{
    if (!textureLoaded)
    {
        return;
    }
    SDL_DestroyTexture(sdlTexture);
    SDL_DestroySurface(sdlSurface);
    textureLoaded = false;
}

void openGtexCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    destroyExistingTexture();
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    texture = TextureAsset::CreateFromAsset(fileList[0]);
    SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(texture.GetWidth()),
                                                 static_cast<int>(texture.GetHeight()),
                                                 SDL_PIXELFORMAT_RGBA8888,
                                                 const_cast<uint32_t *>(texture.GetPixels()),
                                                 static_cast<int>(texture.GetWidth() * sizeof(uint)));
    if (surface == nullptr)
    {
        printf("SDL_CreateSurfaceFrom() failed: %s\n", SDL_GetError());
        return;
    }
    sdlTexture = SDL_CreateTextureFromSurface(renderer, surface);
    if (sdlTexture == nullptr)
    {
        printf("SDL_CreateTextureFromSurface() failed: %s\n", SDL_GetError());
        return;
    }
    SDL_SetTextureScaleMode(sdlTexture, SDL_SCALEMODE_NEAREST);
    sdlSurface = surface;
    textureLoaded = true;
}

void importCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    destroyExistingTexture();
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    texture = TextureAsset::CreateFromImage(fileList[0]);
    SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(texture.GetWidth()),
                                                 static_cast<int>(texture.GetHeight()),
                                                 SDL_PIXELFORMAT_RGBA8888,
                                                 const_cast<uint32_t *>(texture.GetPixels()),
                                                 static_cast<int>(texture.GetWidth() * sizeof(uint)));
    if (surface == nullptr)
    {
        printf("SDL_CreateSurfaceFrom() failed: %s\n", SDL_GetError());
        return;
    }
    sdlTexture = SDL_CreateTextureFromSurface(renderer, surface);
    if (sdlTexture == nullptr)
    {
        printf("SDL_CreateTextureFromSurface() failed: %s\n", SDL_GetError());
        return;
    }
    sdlSurface = surface;
    textureLoaded = true;
}

void saveGtexCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    texture.SaveAsAsset(fileList[0]);
}

void exportCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    texture.SaveAsImage(fileList[0], TextureAsset::ImageFormat::IMAGE_FORMAT_PNG);
}

static void Render(bool &done, SDL_Window *window)
{
    static float zoom = 1.0f;

    ImGui::Begin("texedit",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    bool importPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_O);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && textureLoaded;
    bool exportPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S) && textureLoaded;

    bool zoomInPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal) && textureLoaded;
    bool zoomOutPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus) && textureLoaded;
    bool resetZoomPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_0) && textureLoaded;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            importPressed |= ImGui::MenuItem("Import", "Ctrl+Shift+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S", false, textureLoaded);
            exportPressed |= ImGui::MenuItem("Export", "Ctrl+Shift+S", false, textureLoaded);
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                done = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View", textureLoaded))
        {
            zoomInPressed |= ImGui::MenuItem("Zoom In", "Ctrl+=");
            zoomOutPressed |= ImGui::MenuItem("Zoom Out", "Ctrl+-");
            resetZoomPressed |= ImGui::MenuItem("Reset Zoom", "Ctrl+0");
            ImGui::EndMenu();
        }
        SharedMgr::SharedMenuUI();
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDL_ShowOpenFileDialog(openGtexCallback, nullptr, window, {&gtexFilter}, 1, nullptr, false);
    } else if (importPressed)
    {
        SDL_ShowOpenFileDialog(importCallback, nullptr, window, imageFilters.data(), 4, nullptr, false);
    } else if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveGtexCallback, nullptr, window, {&gtexFilter}, 1, nullptr);
    } else if (exportPressed)
    {
        SDL_ShowSaveFileDialog(exportCallback, nullptr, window, {&pngFilter}, 1, nullptr);
    } else if (zoomInPressed)
    {
        zoom += 0.1;
        if (zoom > 5.0)
        {
            zoom = 5.0;
        }
    } else if (zoomOutPressed)
    {
        zoom -= 0.1;
        if (zoom < 0.1)
        {
            zoom = 0.1;
        }
    } else if (resetZoomPressed)
    {
        zoom = 1.0f;
    }

    if (textureLoaded)
    {
        const ImVec2 &availableSize = ImGui::GetContentRegionAvail();

        constexpr float statsWidth = 150.0f;
        const float imageWidth = availableSize.x - statsWidth - 8.0f;

        ImGui::BeginChild("ImagePane",
                          ImVec2(imageWidth, availableSize.y),
                          ImGuiChildFlags_Border,
                          ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
        {
            const ImVec2 imageSize{static_cast<float>(texture.GetWidth()) * zoom,
                                   static_cast<float>(texture.GetHeight()) * zoom};
            ImGui::Image(sdlTexture, imageSize);
        }
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("StatsPane", ImVec2(statsWidth, availableSize.y));
        {
            ImGui::Text("Stats:");
            ImGui::Separator();
            ImGui::Text("Width: %d\nHeight: %d\nMemory: %lu B",
                        texture.GetWidth(),
                        texture.GetHeight(),
                        texture.GetWidth() * texture.GetHeight() * sizeof(uint32_t));
        }
        ImGui::EndChild();

    } else
    {
        ImGui::TextDisabled("No texture is open. Open or import one from the File menu.");
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
    SDL_Window *window = SDL_CreateWindow("texedit", 800, 600, windowFlags);
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
    destroyExistingTexture();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
