#include <array>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <iostream>
#include <SDL3/SDL.h>
#include "imgui_internal.h"
#include "libassets/TextureAsset.h"

static TextureAsset texture;
static bool texture_loaded = false;
SDL_Renderer *renderer;
SDL_Texture *sdltexture;

void openGtexCallback(void * /*userdata*/, const char *const *filelist, int  /*filter*/)
{
    if (filelist == nullptr || filelist[0] == nullptr) return;
    texture = TextureAsset(static_cast<const char*>(filelist[0]));
    texture.FinishLoading();
    SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(texture.GetWidth()),
                          static_cast<int>(texture.GetHeight()),
                          SDL_PIXELFORMAT_RGBA8888,
                          texture.GetPixels(),
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
    texture_loaded = true;
}

void importCallback(void*  /*userdata*/, const char *const *filelist, int  /*filter*/)
{
    if (filelist == nullptr || filelist[0] == nullptr) return;
    int _;
    texture = TextureAsset(static_cast<const char*>(filelist[0]), &_);
    texture.FinishLoading();
    SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(texture.GetWidth()),
                          static_cast<int>(texture.GetHeight()),
                          SDL_PIXELFORMAT_RGBA8888,
                          texture.GetPixels(),
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
    texture_loaded = true;
}

void saveGtexCallback(void * /*userdata*/, const char *const *filelist, int  /*filter*/)
{
    if (filelist == nullptr || filelist[0] == nullptr) return;
    texture.SaveToFile(filelist[0]);
}

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

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
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

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
            constexpr SDL_DialogFileFilter gtexFilter = {"GAME texture (*.gtex)", "gtex"};
            constexpr SDL_DialogFileFilter filters[] = {
                {
                    "Images",
                    "png;jpg;jpeg;tga"
                },
                {
                    "PNG Images",
                    "png"
                },
                {
                    "JPG Images",
                    "jpg;jepg"
                },
                {
                    "TGA Images",
                    "tga"
                }
            };

            ImGui::Begin("texedit",
                         nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Open", "Ctrl+O"))
                    {
                        SDL_ShowOpenFileDialog(openGtexCallback, nullptr, window, {&gtexFilter}, 1, nullptr, false);
                    }
                    if (ImGui::MenuItem("Import", "Ctrl+Shift+O"))
                    {
                        SDL_ShowOpenFileDialog(importCallback, nullptr, window, filters, 4, nullptr, false);
                    }
                    if (ImGui::MenuItem("Save", "Ctrl+S"))
                    {
                        SDL_ShowSaveFileDialog(saveGtexCallback, nullptr, window, {&gtexFilter}, 1, nullptr);
                    }
                    if (ImGui::MenuItem("Export", "Ctrl+Shift+S"))
                    {}
                    ImGui::Separator();
                    if (ImGui::MenuItem("Quit", "Alt+F4"))
                    {
                        done = true;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            if (texture_loaded)
            {
                const ImVec2 availableSize = ImGui::GetContentRegionAvail();

                constexpr float statsWidth = 150.0f;
                const float imageWidth = availableSize.x - statsWidth - 8.0f;

                ImGui::BeginChild("ImagePane", ImVec2(imageWidth, availableSize.y), ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar);
                {
                    const ImVec2 imageSize = ImVec2(texture.GetWidth(), texture.GetHeight());
                    ImGui::Image(sdltexture, imageSize);
                }
                ImGui::EndChild();
                ImGui::SameLine();

                ImGui::BeginChild("StatsPane", ImVec2(statsWidth, availableSize.y));
                {
                    ImGui::Text("Stats:");
                    ImGui::Separator();
                    ImGui::Text("Width: %d\nHeight: %d\nMemory: %d B", texture.GetWidth(), texture.GetHeight(), texture.GetWidth() * texture.GetHeight() * sizeof(uint32_t));
                }
                ImGui::EndChild();

            } else
            {
                ImGui::TextDisabled("No texture is open. Open or import one from the File menu.");
            }

            ImGui::End();
        }

        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 1);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
