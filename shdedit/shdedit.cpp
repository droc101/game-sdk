//
// Created by droc101 on 7/23/25.
//
#include <array>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <misc/cpp/imgui_stdlib.h>
#include <iostream>
#include <libassets/asset/TextureAsset.h>
#include <SDL3/SDL.h>
#include "libassets/asset/ShaderAsset.h"
#include "Options.h"
#include "SDLRendererImGuiTextureAssetCache.h"
#include "SharedMgr.h"

static ShaderAsset shader;
static bool shaderLoaded = false;
SDL_Renderer *renderer;
SDL_Window *window;

constexpr SDL_DialogFileFilter gshdFilter = {"GAME shader (*.gshd)", "gshd"};
constexpr std::array glslFilters = {
    SDL_DialogFileFilter{"GLSL source files (*.glsl, *.vert, *.frag)", "glsl;vert;frag"},
    SDL_DialogFileFilter{"GLSL source (*.glsl)", "glsl"},
    SDL_DialogFileFilter{"GLSL fragment (*.frag)", "frag"},
    SDL_DialogFileFilter{"GLSL vertex (*.vert)", "vert"},
};

void openGfonCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode e = ShaderAsset::CreateFromAsset(fileList[0], shader);
    if (e != Error::ErrorCode::E_OK)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", std::format("Failed to open the shader!\n{}", Error::ErrorString(e)).c_str(), window);
        return;
    }
    shaderLoaded = true;
}

void importCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode e = ShaderAsset::CreateFromGlsl(fileList[0], shader);
    if (e != Error::ErrorCode::E_OK)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", std::format("Failed to import the shader!\n{}", Error::ErrorString(e)).c_str(), window);
        return;
    }
    shaderLoaded = true;
}

void saveGfonCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = shader.SaveAsAsset(fileList[0]);
    if (errorCode != Error::ErrorCode::E_OK)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", std::format("Failed to save the shader!\n{}", Error::ErrorString(errorCode)).c_str(), window);
    }
}

void exportCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = shader.SaveAsGlsl(fileList[0]);
    if (errorCode != Error::ErrorCode::E_OK)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", std::format("Failed to export the shader!\n{}", Error::ErrorString(errorCode)).c_str(), window);
    }
}

static void Render(bool &done, SDL_Window *window)
{
    ImGui::Begin("shdedit",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);
    bool newPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    bool importPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_O);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && shaderLoaded;
    bool exportPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S) && shaderLoaded;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            newPressed |= ImGui::MenuItem("New", "Ctrl+N");
            ImGui::Separator();
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            importPressed |= ImGui::MenuItem("Import", "Ctrl+Shift+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S", false, shaderLoaded);
            exportPressed |= ImGui::MenuItem("Export", "Ctrl+Shift+S", false, shaderLoaded);
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
        SDL_ShowOpenFileDialog(openGfonCallback, nullptr, window, {&gshdFilter}, 1, nullptr, false);
    } else if (importPressed)
    {
        SDL_ShowOpenFileDialog(importCallback, nullptr, window, glslFilters.data(), 4, nullptr, false);
    } else if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveGfonCallback, nullptr, window, {&gshdFilter}, 1, nullptr);
    } else if (exportPressed)
    {
        SDL_ShowSaveFileDialog(exportCallback, nullptr, window, glslFilters.data(), 4, nullptr);
    } else if (newPressed)
    {
        shader = ShaderAsset();
        shaderLoaded = true;
    }

    if (shaderLoaded)
    {
        const ImVec2 &availableSize = ImGui::GetContentRegionAvail();

        constexpr float statsWidth = 150.0f;
        const float imageWidth = availableSize.x - statsWidth - 8.0f;

        ImGui::BeginChild("ImagePane",
                          ImVec2(imageWidth, availableSize.y),
                          ImGuiChildFlags_Border,
                          ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);
        {
            ImGui::InputTextMultiline("##glsl", &shader.GetGLSL(), ImVec2(-1, -1));
        }
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("StatsPane", ImVec2(statsWidth, availableSize.y));
        {
            ImGui::Text("Platform");
            if (ImGui::RadioButton("Vulkan", shader.platform == ShaderAsset::ShaderPlatform::PLATFORM_VULKAN)) shader.platform = ShaderAsset::ShaderPlatform::PLATFORM_VULKAN;
            if (ImGui::RadioButton("OpenGL", shader.platform == ShaderAsset::ShaderPlatform::PLATFORM_OPENGL)) shader.platform = ShaderAsset::ShaderPlatform::PLATFORM_OPENGL;

            ImGui::Text("Type");
            if (ImGui::RadioButton("Fragment", shader.type == ShaderAsset::ShaderType::SHADER_TYPE_FRAG)) shader.type = ShaderAsset::ShaderType::SHADER_TYPE_FRAG;
            if (ImGui::RadioButton("Vertex", shader.type == ShaderAsset::ShaderType::SHADER_TYPE_VERT)) shader.type = ShaderAsset::ShaderType::SHADER_TYPE_VERT;
        }
        ImGui::EndChild();
    } else
    {
        ImGui::TextDisabled("No shader is open. Open, create, or import one from the File menu.");
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
    window = SDL_CreateWindow("shdedit", 800, 600, windowFlags);
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

    SDLRendererImGuiTextureAssetCache cache = SDLRendererImGuiTextureAssetCache(renderer);
    SharedMgr::InitSharedMgr(&cache);

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
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
