#include <array>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <iostream>
#include <SDL3/SDL.h>
#include "AboutWindow.h"
#include "imgui_internal.h"
#include "Options.h"
#include "OptionsWindow.h"
#include "libassets/TextureAsset.h"

static TextureAsset texture;
static bool texture_loaded = false;
SDL_Renderer *renderer;
SDL_Texture *sdltexture;
SDL_Surface *sdlsurface;

constexpr SDL_DialogFileFilter gtexFilter = {"GAME texture (*.gtex)", "gtex"};
constexpr SDL_DialogFileFilter pngFilter = {"PNG Image", "png"};
constexpr std::array imageFilters = {
        SDL_DialogFileFilter{
                "Images",
                "png;jpg;jpeg;tga"
        },
        SDL_DialogFileFilter{
                "PNG Images",
                "png"
        },
        SDL_DialogFileFilter{
                "JPG Images",
                "jpg;jepg"
        },
        SDL_DialogFileFilter{
                "TGA Images",
                "tga"
        }
};

void destroyExistingTexture()
{
    if (!texture_loaded) return;
    SDL_DestroyTexture(sdltexture);
    SDL_DestroySurface(sdlsurface);
    texture_loaded = false;
}

void openGtexCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/)
{
    destroyExistingTexture();
    if (filelist == nullptr || filelist[0] == nullptr) return;
    texture = TextureAsset::CreateFromAsset(filelist[0]);
    SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(texture.GetWidth()),
                                                 static_cast<int>(texture.GetHeight()),
                                                 SDL_PIXELFORMAT_RGBA8888,
                                                 (void *)texture.GetPixels(),
                                                 static_cast<int>(texture.GetWidth() * sizeof(uint)));
    if (surface == nullptr)
    {
        printf("SDL_CreateSurfaceFrom() failed: %s\n", SDL_GetError());
        return;
    }
    sdltexture = SDL_CreateTextureFromSurface(renderer, surface);
    if (sdltexture == nullptr)
    {
        printf("SDL_CreateTextureFromSurface() failed: %s\n", SDL_GetError());
        return;
    }
    sdlsurface = surface;
    texture_loaded = true;
}

void importCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/)
{
    destroyExistingTexture();
    if (filelist == nullptr || filelist[0] == nullptr) return;
    texture = TextureAsset::CreateFromImage(filelist[0]);
    SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(texture.GetWidth()),
                                                 static_cast<int>(texture.GetHeight()),
                                                 SDL_PIXELFORMAT_RGBA8888,
                                                 (void *)texture.GetPixels(),
                                                 static_cast<int>(texture.GetWidth() * sizeof(uint)));
    if (surface == nullptr)
    {
        printf("SDL_CreateSurfaceFrom() failed: %s\n", SDL_GetError());
        return;
    }
    sdltexture = SDL_CreateTextureFromSurface(renderer, surface);
    if (sdltexture == nullptr)
    {
        printf("SDL_CreateTextureFromSurface() failed: %s\n", SDL_GetError());
        return;
    }
    sdlsurface = surface;
    texture_loaded = true;
}

void saveGtexCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/)
{
    if (filelist == nullptr || filelist[0] == nullptr) return;
    texture.SaveAsAsset(filelist[0]);
}

void exportCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/)
{
    if (filelist == nullptr || filelist[0] == nullptr) return;
    texture.SaveAsImage(filelist[0], TextureAsset::IMAGE_FORMAT_PNG);
}

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    Options::Load();

    constexpr SDL_WindowFlags window_flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
    SDL_Window *window = SDL_CreateWindow("texedit", 800, 600, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    if (renderer == nullptr)
    {
        SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
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
        {

            static float zoom = 1.0f;

            ImGui::Begin("texedit",
                         nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
            bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
            bool importPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_O);
            bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && texture_loaded;
            bool exportPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S) && texture_loaded;

            bool zoomInPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal) && texture_loaded;
            bool zoomOutPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus) && texture_loaded;
            bool resetZoomPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_0) && texture_loaded;

            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
                    importPressed |= ImGui::MenuItem("Import", "Ctrl+Shift+O");
                    savePressed |= ImGui::MenuItemEx("Save", nullptr, "Ctrl+S", false, texture_loaded);
                    exportPressed |= ImGui::MenuItemEx("Export", nullptr, "Ctrl+Shift+S", false, texture_loaded);
                    ImGui::Separator();
                    if (ImGui::MenuItem("Quit", "Alt+F4"))
                    {
                        done = true;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenuEx("View", nullptr, texture_loaded))
                {
                    zoomInPressed |= ImGui::MenuItem("Zoom In", "Ctrl+=");
                    zoomOutPressed |= ImGui::MenuItem("Zoom Out", "Ctrl+-");
                    resetZoomPressed |= ImGui::MenuItem("Reset Zoom", "Ctrl+0");
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Tools"))
                {
                    if (ImGui::MenuItem("Options")) OptionsWindow::Show();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Help"))
                {
                    if (ImGui::MenuItem("Source Code")) SDL_OpenURL("https://github.com/droc101/game-sdk");
                    if (ImGui::MenuItem("About")) AboutWindow::Show();
                    ImGui::EndMenu();
                }
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

            if (texture_loaded)
            {
                const ImVec2 availableSize = ImGui::GetContentRegionAvail();

                constexpr float statsWidth = 150.0f;
                const float imageWidth = availableSize.x - statsWidth - 8.0f;

                ImGui::BeginChild("ImagePane",
                                  ImVec2(imageWidth, availableSize.y),
                                  ImGuiChildFlags_Border,
                                  ImGuiWindowFlags_HorizontalScrollbar |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus);
                {
                    const ImVec2 imageSize = ImVec2(static_cast<float>(texture.GetWidth()) * zoom,
                                                    static_cast<float>(texture.GetHeight()) * zoom);
                    ImGui::Image(sdltexture, imageSize);
                }
                ImGui::EndChild();
                ImGui::SameLine();

                ImGui::BeginChild("StatsPane", ImVec2(statsWidth, availableSize.y));
                {
                    ImGui::Text("Stats:");
                    ImGui::Separator();
                    ImGui::Text("Width: %d\nHeight: %d\nMemory: %d B",
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

        OptionsWindow::Render(window);
        AboutWindow::Render(window);

        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 1);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    Options::Save();

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    destroyExistingTexture();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
