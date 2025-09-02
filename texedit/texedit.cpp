#include <array>
#include <cstdint>
#include <cstdio>
#include <format>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <libassets/asset/TextureAsset.h>
#include <libassets/util/Error.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include "DialogFilters.h"
#include "SDLRendererImGuiTextureAssetCache.h"
#include "SharedMgr.h"

static TextureAsset texture{};
static bool textureLoaded = false;
static SDL_Renderer *renderer = nullptr;
static SDL_Window *window = nullptr;
static SDL_Texture *sdlTexture = nullptr;
static SDL_Surface *sdlSurface = nullptr;
static float zoom = 1.0f;


static inline void destroyExistingTexture()
{
    if (!textureLoaded)
    {
        return;
    }
    SDL_DestroyTexture(sdlTexture);
    SDL_DestroySurface(sdlSurface);
    textureLoaded = false;
}

static inline bool loadTexture()
{
    destroyExistingTexture();
    SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(texture.GetWidth()),
                                                 static_cast<int>(texture.GetHeight()),
                                                 SDL_PIXELFORMAT_RGBA8888,
                                                 texture.GetPixels(),
                                                 static_cast<int>(texture.GetWidth() * sizeof(uint32_t)));
    if (surface == nullptr)
    {
        return false;
    }
    sdlTexture = SDL_CreateTextureFromSurface(renderer, surface);
    if (sdlTexture == nullptr)
    {
        return false;
    }
    if (!SDL_SetTextureScaleMode(sdlTexture, SDL_SCALEMODE_NEAREST))
    {
        printf("Error: SDL_SetTextureScaleMode(): %s\n", SDL_GetError());
        return false;
    }
    sdlSurface = surface;
    textureLoaded = true;
    return true;
}

static void openGtexCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = TextureAsset::CreateFromAsset(fileList[0], texture);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to open the texture!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
        return;
    }
    if (!loadTexture())
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to load the texture!\n{}", SDL_GetError()).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
    }
}

static void importCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = TextureAsset::CreateFromImage(fileList[0], texture);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to import the texture!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
        return;
    }
    if (!loadTexture())
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to load the texture!\n{}", SDL_GetError()).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
    }
}

static void saveGtexCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = texture.SaveAsAsset(fileList[0]);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to save the texture!\n{}", errorCode).c_str(),
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
    const Error::ErrorCode errorCode = texture.SaveAsImage(fileList[0], TextureAsset::ImageFormat::IMAGE_FORMAT_PNG);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to export the texture!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
    }
}

static void Render(bool &done, SDL_Window *sdlWindow)
{
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("texedit", nullptr, windowFlags);
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
        SharedMgr::SharedMenuUI("texedit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDL_ShowOpenFileDialog(openGtexCallback,
                               nullptr,
                               sdlWindow,
                               DialogFilters::gtexFilters.data(),
                               1,
                               nullptr,
                               false);
    } else if (importPressed)
    {
        SDL_ShowOpenFileDialog(importCallback,
                               nullptr,
                               sdlWindow,
                               DialogFilters::imageFilters.data(),
                               4,
                               nullptr,
                               false);
    } else if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveGtexCallback, nullptr, sdlWindow, DialogFilters::gtexFilters.data(), 1, nullptr);
    } else if (exportPressed)
    {
        SDL_ShowSaveFileDialog(exportCallback, nullptr, sdlWindow, DialogFilters::pngFilters.data(), 1, nullptr);
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
            ImGui::TextUnformatted(std::format("Width: {}\nHeight: {}\nMemory: {} B",
                                               texture.GetWidth(),
                                               texture.GetHeight(),
                                               texture.GetWidth() * texture.GetHeight() * sizeof(uint32_t))
                    .c_str());

            ImGui::Separator();
            ImGui::Checkbox("Filter", &texture.filter); // TODO live update
            ImGui::Checkbox("Repeat", &texture.repeat);
            ImGui::Checkbox("Mipmaps", &texture.mipmaps);
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
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow("texedit", 800, 600, windowFlags);
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
    destroyExistingTexture();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
