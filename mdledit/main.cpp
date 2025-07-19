#include <array>
#include <GL/glew.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <iostream>
#include <SDL3/SDL.h>
#include "LodEditWindow.h"
#include <libassets/asset/ModelAsset.h>
#include "MdleditImGuiTextureCache.h"
#include "ModelRenderer.h"
#include "SharedMgr.h"
#include "SkinEditWindow.h"

static bool modelLoaded = false;
static bool dragging = false;

SDL_Window *window = nullptr;
SDL_GLContext glContext = nullptr;
ImGuiIO io;
bool done = false;
bool openPressed = false;
bool newPressed = false;
bool savePressed = false;
bool exportPressed = false;

constexpr SDL_DialogFileFilter gmdlFilter = {"GAME model (*.gmdl)", "gmdl"};
constexpr std::array modelFilters = {
        SDL_DialogFileFilter{"3D Models (obj, fbx, gltf, dae)", "obj;fbx;gltf;dae"},
        SDL_DialogFileFilter{"Wavefront OBJ Models", "obj"},
        SDL_DialogFileFilter{"FBX Models", "fbx"},
        SDL_DialogFileFilter{"glTF/glTF2.0 Models", "gltf"},
        SDL_DialogFileFilter{"Collada Models", "dae"},
};


void destroyExistingModel()
{
    if (!modelLoaded)
    {
        return;
    }
    ModelRenderer::UnloadModel();
    modelLoaded = false;
}

void openGmdlCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    char *path = strdup(fileList[0]);
    SDL_Event e{};
    e.type = ModelRenderer::EVENT_RELOAD_MODEL;
    e.user.code = ModelRenderer::EVENT_RELOAD_MODEL_CODE_GMDL;
    e.user.data1 = path;
    SDL_PushEvent(&e);
}

void importCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    char *path = strdup(fileList[0]);
    SDL_Event e{};
    e.type = ModelRenderer::EVENT_RELOAD_MODEL;
    e.user.code = ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_MODEL;
    e.user.data1 = path;
    SDL_PushEvent(&e);
}

void saveGmdlCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    assert(ModelRenderer::GetModel()->SaveAsAsset(fileList[0]) == Error::ErrorCode::E_OK);
}

void ProcessEvent(const SDL_Event *event)
{
    ImGui_ImplSDL3_ProcessEvent(event);
    io = ImGui::GetIO();
    if (event->type == ModelRenderer::EVENT_RELOAD_MODEL)
    {
        destroyExistingModel();
        ModelAsset model;
        const char *path = static_cast<char *>(event->user.data1);
        if (event->user.code == ModelRenderer::EVENT_RELOAD_MODEL_CODE_GMDL)
        {
            assert(ModelAsset::CreateFromAsset(path, model) == Error::ErrorCode::E_OK);
        } else if (event->user.code == ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_MODEL)
        {
            assert(ModelAsset::CreateFromStandardModel(path, model) == Error::ErrorCode::E_OK);
        } else if (event->user.code == ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_LOD)
        {
            model = *ModelRenderer::GetModel();
            model.AddLod(path);
        }
        ModelRenderer::LoadModel(model);
        modelLoaded = true;
        delete path;
    }
    if (event->type == ModelRenderer::EVENT_SAVE_MODEL)
    {
        const char *path = static_cast<char *>(event->user.data1);
        assert(ModelRenderer::GetModel()->SaveAsAsset(path) == Error::ErrorCode::E_OK);
        delete path;
    }
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
            const SDL_MouseWheelEvent &e = event->wheel;
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
    newPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N);
    openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O);
    savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && modelLoaded;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            newPressed |= ImGui::MenuItem("New", "Ctrl+N");
            ImGui::Separator();
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S", false, modelLoaded);
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                done = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit", modelLoaded))
        {
            if (ImGui::MenuItem("LOD Editor")) LodEditWindow::Show();
            if (ImGui::MenuItem("Skin Editor")) SkinEditWindow::Show();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View", modelLoaded))
        {
            if (ImGui::MenuItem("Reset View"))
            {
                ModelRenderer::UpdateView(0, 0, 1);
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Display Mode"))
            {
                if (ImGui::MenuItem("Unshaded", "", ModelRenderer::displayMode == ModelRenderer::DisplayMode::COLORED))
                {
                    ModelRenderer::displayMode = ModelRenderer::DisplayMode::COLORED;
                }
                if (ImGui::MenuItem("Shaded",
                                    "",
                                    ModelRenderer::displayMode == ModelRenderer::DisplayMode::COLORED_SHADED))
                {
                    ModelRenderer::displayMode = ModelRenderer::DisplayMode::COLORED_SHADED;
                }
                if (ImGui::MenuItem("Textured Unshaded",
                                    "",
                                    ModelRenderer::displayMode == ModelRenderer::DisplayMode::TEXTURED))
                {
                    ModelRenderer::displayMode = ModelRenderer::DisplayMode::TEXTURED;
                }
                if (ImGui::MenuItem("Textured Shaded",
                                    "",
                                    ModelRenderer::displayMode == ModelRenderer::DisplayMode::TEXTURED_SHADED))
                {
                    ModelRenderer::displayMode = ModelRenderer::DisplayMode::TEXTURED_SHADED;
                }
                if (ImGui::MenuItem("UV Debug", "", ModelRenderer::displayMode == ModelRenderer::DisplayMode::UV))
                {
                    ModelRenderer::displayMode = ModelRenderer::DisplayMode::UV;
                }
                if (ImGui::MenuItem("Normal Debug",
                                    "",
                                    ModelRenderer::displayMode == ModelRenderer::DisplayMode::NORMAL))
                {
                    ModelRenderer::displayMode = ModelRenderer::DisplayMode::NORMAL;
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Show Backfaces", "", !ModelRenderer::cullBackfaces))
            {
                ModelRenderer::cullBackfaces = !ModelRenderer::cullBackfaces;
            }
            if (ImGui::MenuItem("Show Unit Cube", "", ModelRenderer::showUnitCube))
            {
                ModelRenderer::showUnitCube = !ModelRenderer::showUnitCube;
            }
            if (ImGui::MenuItem("Wireframe", "", ModelRenderer::wireframe))
            {
                ModelRenderer::wireframe = !ModelRenderer::wireframe;
            }
            ImGui::EndMenu();
        }
        SharedMgr::SharedMenuUI();
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDL_ShowOpenFileDialog(openGmdlCallback, nullptr, window, {&gmdlFilter}, 1, nullptr, false);
    } else if (newPressed)
    {
        SDL_ShowOpenFileDialog(importCallback,
                               nullptr,
                               window,
                               modelFilters.data(),
                               modelFilters.size(),
                               nullptr,
                               false);
    } else if (savePressed)
    {
        if (!ModelRenderer::GetModel()->ValidateLodDistances())
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                     "Invalid Model",
                                     "LOD distances are invalid! Please fix them in the LOD editor and make sure that:\n- The first LOD (LOD 0) has a distance of 0\n- No two LODs have the same distance",
                                     window);
        } else
        {
            SDL_ShowSaveFileDialog(saveGmdlCallback, nullptr, window, {&gmdlFilter}, 1, nullptr);
        }
    }
}

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    MdleditImGuiTextureCache cache = MdleditImGuiTextureCache();
    SharedMgr::InitSharedMgr(&cache);

    const char *glslVersion = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

    constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow("mdledit", 800, 600, windowFlags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    glContext = SDL_GL_CreateContext(window);
    if (glContext == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }

    ModelRenderer::EVENT_RELOAD_MODEL = SDL_RegisterEvents(2);
    ModelRenderer::EVENT_SAVE_MODEL = ModelRenderer::EVENT_RELOAD_MODEL + 1;

    SDL_GL_MakeCurrent(window, glContext);
    ModelRenderer::Init();
    SDL_GL_SetSwapInterval(1); // Enable vsync
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    if (SDL_GetSystemTheme() == SDL_SYSTEM_THEME_DARK)
    {
        ImGui::StyleColorsDark();
    } else
    {
        ImGui::StyleColorsLight();
    }

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(glslVersion);

    ModelRenderer::ResizeWindow(800, 600);

    // ReSharper disable once CppDFALoopConditionNotUpdated Wrong again
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
        constexpr float modelPaneSize = ModelRenderer::PANEL_SIZE;
        const ImVec2 workSize{viewport->WorkSize.x, modelPaneSize};
        const ImVec2 workPos{viewport->WorkPos.x, (viewport->WorkPos.y + viewport->WorkSize.y) - modelPaneSize};
        ImGui::SetNextWindowPos(workPos);
        ImGui::SetNextWindowSize(workSize);
        {
            ImGui::Begin("mdledit",
                         nullptr,
                         ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoBringToFrontOnFocus);

            HandleMenuAndShortcuts();

            if (modelLoaded)
            {
                ImGui::PushItemWidth(-1);
                ImGui::Text("LOD");
                ImGui::SliderInt("##LOD",
                                 &ModelRenderer::lod,
                                 0,
                                 static_cast<int>(ModelRenderer::GetModel()->GetLodCount() - 1));
                ImGui::Text("Skin");
                ImGui::SliderInt("##Skin",
                                 &ModelRenderer::skin,
                                 0,
                                 static_cast<int>(ModelRenderer::GetModel()->GetSkinCount() - 1));
            } else
            {
                ImGui::TextDisabled("No model is open. Open or create one from the File menu.");
            }

            ImGui::End();
        }

        SharedMgr::RenderSharedUI(window);
        SkinEditWindow::Render();
        LodEditWindow::Render(window);

        ImGui::Render();
        glClearColor(0, 0, 0, 1);
        if (modelLoaded)
        {
            ModelRenderer::Render();
        } else
        {
            glClear(GL_COLOR_BUFFER_BIT);
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    SharedMgr::DestroySharedMgr();

    ModelRenderer::Destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    destroyExistingModel();
    return 0;
}
