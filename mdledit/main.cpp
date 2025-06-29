#include <array>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "imgui_internal.h"
#include "libassets/ModelAsset.h"
#include <GLES3/gl3.h>
#include "../shared/AboutWindow.h"
#include "ModelRenderer.h"
#include "../shared/Options.h"
#include "OptionsWindow.h"

static bool model_loaded = false;
static bool dragging = false;

SDL_Window *window = nullptr;
SDL_GLContext gl_context = nullptr;
ImGuiIO io;
bool done = false;
bool openPressed = false;
bool importPressed = false;
bool savePressed = false;
bool exportPressed = false;

constexpr SDL_DialogFileFilter gmdlFilter = {"GAME model (*.gmdl)", "gmdl"};
constexpr SDL_DialogFileFilter objFilter = {"OBJ model", "obj"};

void destroyExistingModel()
{
    if (!model_loaded) return;
    ModelRenderer::UnloadModel();
    model_loaded = false;
}

void openGmdlCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/)
{
    destroyExistingModel();
    if (filelist == nullptr || filelist[0] == nullptr) return;
    ModelRenderer::LoadModel(filelist[0]);
    model_loaded = true;
}

void importCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/)
{
    if (filelist == nullptr || filelist[0] == nullptr) return;
    // TODO: implement OBJ loader in ModelAsset
}

void saveGmdlCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/)
{
    if (filelist == nullptr || filelist[0] == nullptr) return;
    ModelRenderer::GetModel()->SaveAsAsset(filelist[0]);
}

void exportCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/)
{
    if (filelist == nullptr || filelist[0] == nullptr) return;
    // TODO: implement OBJ writer in ModelAsset
}

void ProcessEvent(const SDL_Event *event)
{
    ImGui_ImplSDL3_ProcessEvent(event);
    io = ImGui::GetIO();
    if (event->type == SDL_EVENT_QUIT)
    {
        done = true;
    }
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event->window.windowID == SDL_GetWindowID(window))
    {
        done = true;
    }
    if (event->type == SDL_EVENT_WINDOW_RESIZED)
    {
        int w;
        int h;
        SDL_GetWindowSize(window, &w, &h);
        ModelRenderer::ResizeWindow(w, h);
    }
    if (!io.WantCaptureMouse)
    {
        if (event->type == SDL_EVENT_MOUSE_WHEEL)
        {
            SDL_MouseWheelEvent const e = event->wheel;
            ModelRenderer::UpdateViewRel(0, 0, e.y / -10.0f);
        }
        if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event->button.button == SDL_BUTTON_LEFT)
            {
                dragging = false;
            }
        } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            if (event->button.button == SDL_BUTTON_LEFT)
            {
                dragging = true;
            }
        }
        if (dragging && event->type == SDL_EVENT_MOUSE_MOTION)
        {
            ModelRenderer::UpdateViewRel(event->motion.yrel / 5.0f, event->motion.xrel / -5.0f, 0);
        }
    }
}

void HandleMenuAndShortcuts()
{
    openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    importPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_O);
    savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && model_loaded;
    exportPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S) && model_loaded;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            importPressed |= ImGui::MenuItem("Import", "Ctrl+Shift+O");
            savePressed |= ImGui::MenuItemEx("Save", nullptr, "Ctrl+S", false, model_loaded);
            exportPressed |= ImGui::MenuItemEx("Export", nullptr, "Ctrl+Shift+S", false, model_loaded);
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                done = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenuEx("Edit", "", model_loaded))
        {
            ImGui::MenuItem("Import LOD");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenuEx("View", "", model_loaded))
        {
            if (ImGui::MenuItem("Reset View"))
            {
                ModelRenderer::UpdateView(0, 0, 1);
            }
            if (ImGui::MenuItemEx("Backface Culling", "", nullptr, *ModelRenderer::GetCullBackfaces()))
            {
                *ModelRenderer::GetCullBackfaces() = !*ModelRenderer::GetCullBackfaces();
            }
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
        SDL_ShowOpenFileDialog(openGmdlCallback, nullptr, window, {&gmdlFilter}, 1, nullptr, false);
    } else if (importPressed)
    {
        SDL_ShowOpenFileDialog(importCallback, nullptr, window, {&objFilter}, 1, nullptr, false);
    } else if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveGmdlCallback, nullptr, window, {&gmdlFilter}, 1, nullptr);
    } else if (exportPressed)
    {
        SDL_ShowSaveFileDialog(exportCallback, nullptr, window, {&objFilter}, 1, nullptr);
    }
}

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    Options::Load();

    const char *glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    constexpr SDL_WindowFlags window_flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow("mdledit", 800, 600, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO();
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

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ModelRenderer::Init();
    ModelRenderer::ResizeWindow(800, 600);

    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ProcessEvent(&event);
        }

        if ((SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0)
        {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        constexpr float modelPaneSize = 250;
        const ImVec2 workSize = ImVec2(viewport->WorkSize.x, modelPaneSize);
        const ImVec2 workPos = ImVec2(viewport->WorkPos.x,
                                      (viewport->WorkPos.y + viewport->WorkSize.y) - modelPaneSize);
        ImGui::SetNextWindowPos(workPos);
        ImGui::SetNextWindowSize(workSize);
        {
            ImGui::Begin("mdledit",
                         nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoBringToFrontOnFocus);

            HandleMenuAndShortcuts();

            if (model_loaded)
            {
                if (ImGui::BeginTabBar("tabs", 0))
                {
                    if (ImGui::BeginTabItem("Display", nullptr, 0))
                    {
                        ImGui::SliderInt("LOD",
                                         ModelRenderer::GetLOD(),
                                         0,
                                         ModelRenderer::GetModel()->GetLodCount() - 1);
                        ImGui::SliderInt("Skin",
                                         ModelRenderer::GetSkin(),
                                         0,
                                         ModelRenderer::GetModel()->GetSkinCount() - 1);
                        ImGui::EndTabItem();
                    }
                    // if (ImGui::BeginTabItem("Skins", nullptr, 0))
                    // {
                    //     ImGui::EndTabItem();
                    // }
                    // if (ImGui::BeginTabItem("LODs", nullptr, 0))
                    // {
                    //     ImGui::EndTabItem();
                    // }
                }
                ImGui::EndTabBar();


            } else
            {
                ImGui::TextDisabled("No model is open. Open or import one from the File menu.");
            }

            ImGui::End();
        }

        OptionsWindow::Render(window);
        AboutWindow::Render(window);

        ImGui::Render();
        glViewport(0, 0, static_cast<GLsizei>(io.DisplaySize.x), static_cast<GLsizei>(io.DisplaySize.y));
        glClearColor(0, 0, 0, 1);
        if (model_loaded) ModelRenderer::Render();
        else glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    Options::Save();

    ModelRenderer::Destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    destroyExistingModel();
    return 0;
}
