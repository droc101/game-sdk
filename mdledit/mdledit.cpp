#include <array>
#include <cstdio>
#include <format>
#include <GL/glew.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/ConvexHull.h>
#include <libassets/util/Error.h>
#include <oneapi/tbb/detail/_utils.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <utility>
#include "CollisionEditWindow.h"
#include "DialogFilters.h"
#include "imgui_internal.h"
#include "LodEditWindow.h"
#include "MaterialEditWindow.h"
#include "ModelRenderer.h"
#include "OpenGLImGuiTextureAssetCache.h"
#include "Options.h"
#include "PreviewOptionsWindow.h"
#include "SharedMgr.h"
#include "SkinEditWindow.h"

static bool modelLoaded = false;
static bool dragging = false;

static SDL_Window *window = nullptr;
static SDL_GLContext glContext = nullptr;
static bool done = false;
static bool openPressed = false;
static bool newPressed = false;
static bool savePressed = false;
static bool previewFocused = false;

static void destroyExistingModel()
{
    if (!modelLoaded)
    {
        return;
    }
    ModelRenderer::UnloadModel();
    modelLoaded = false;
}

static void openGmdlCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    SDL_Event event;
    event.type = ModelRenderer::EVENT_RELOAD_MODEL;
    event.user.code = ModelRenderer::EVENT_RELOAD_MODEL_CODE_GMDL;
    event.user.data1 = new std::string(fileList[0]);
    if (!SDL_PushEvent(&event))
    {
        printf("Error: SDL_PushEvent(): %s\n", SDL_GetError());
    }
}

static void importCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    SDL_Event event;
    event.type = ModelRenderer::EVENT_RELOAD_MODEL;
    event.user.code = ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_MODEL;
    event.user.data1 = new std::string(fileList[0]);
    if (!SDL_PushEvent(&event))
    {
        printf("Error: SDL_PushEvent(): %s\n", SDL_GetError());
    }
}

static void saveGmdlCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = ModelRenderer::GetModel().SaveAsAsset(fileList[0]);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to save the model!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
    }
}

static void ProcessEvent(const SDL_Event *event, ImGuiIO &io)
{
    ImGui_ImplSDL3_ProcessEvent(event);
    io = ImGui::GetIO();
    if (event->type == ModelRenderer::EVENT_RELOAD_MODEL)
    {
        destroyExistingModel();
        ModelAsset model;
        const std::string *path = static_cast<std::string *>(event->user.data1);
        if (event->user.code == ModelRenderer::EVENT_RELOAD_MODEL_CODE_GMDL)
        {
            const Error::ErrorCode errorCode = ModelAsset::CreateFromAsset(*path, model);
            if (errorCode != Error::ErrorCode::OK)
            {
                if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                              "Error",
                                              std::format("Failed to open the model!\n{}", errorCode).c_str(),
                                              window))
                {
                    printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
                }
                delete path;
                return;
            }
        } else if (event->user.code == ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_MODEL)
        {
            const Error::ErrorCode errorCode = ModelAsset::CreateFromStandardModel(*path,
                                                                                   model,
                                                                                   Options::defaultTexture);
            if (errorCode != Error::ErrorCode::OK)
            {
                if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                              "Error",
                                              std::format("Failed to import the model!\n{}", errorCode).c_str(),
                                              window))
                {
                    printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
                }
                delete path;
                return;
            }
        } else if (event->user.code == ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_LOD)
        {
            model = ModelRenderer::GetModel();
            if (!model.AddLod(*path))
            {
                if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                              "Error",
                                              std::format("Failed to import model LOD!").c_str(),
                                              window))
                {
                    printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
                }
                delete path;
                return;
            }
        } else if (event->user.code == ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_HULL)
        {
            model = ModelRenderer::GetModel();
            const ConvexHull hull = ConvexHull(*path);
            model.AddHull(hull);
        } else if (event->user.code == ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_HULL_MULTI)
        {
            model = ModelRenderer::GetModel();
            model.AddHulls(*path);
        } else if (event->user.code == ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_STATIC_COLLIDER)
        {
            model = ModelRenderer::GetModel();
            model.SetStaticCollisionMesh(StaticCollisionMesh(*path));
        }
        ModelRenderer::LoadModel(std::move(model));
        modelLoaded = true;
        delete path;
    }
    if (event->type == ModelRenderer::EVENT_SAVE_MODEL)
    {
        const std::string *path = static_cast<std::string *>(event->user.data1);
        const Error::ErrorCode errorCode = ModelRenderer::GetModel().SaveAsAsset(*path);
        if (errorCode != Error::ErrorCode::OK)
        {
            if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                          "Error",
                                          std::format("Failed to save the model!\n{}", errorCode).c_str(),
                                          window))
            {
                printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
            }
            delete path;
            return;
        }
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
    if (previewFocused)
    {
        if (event->type == SDL_EVENT_MOUSE_WHEEL)
        {
            const SDL_MouseWheelEvent &mouseWheelEvent = event->wheel;
            ModelRenderer::UpdateViewRel(0, 0, mouseWheelEvent.y / -10.0f);
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
    } else
    {
        dragging = false;
    }
}

static void HandleMenuAndShortcuts()
{
    newPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, ImGuiInputFlags_RouteGlobal);
    openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_RouteGlobal);
    savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal) && modelLoaded;

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
            ImGui::Separator();
            if (ImGui::MenuItem("Show Bounding Box", "", ModelRenderer::showBoundingBox))
            {
                ModelRenderer::showBoundingBox = !ModelRenderer::showBoundingBox;
            }
            if (ImGui::MenuItem("Show Collision Model", "", ModelRenderer::showCollisionModel))
            {
                ModelRenderer::showCollisionModel = !ModelRenderer::showCollisionModel;
            }
            ImGui::EndMenu();
        }
        SharedMgr::SharedMenuUI("mdledit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDL_ShowOpenFileDialog(openGmdlCallback, nullptr, window, DialogFilters::gmdlFilters.data(), 1, nullptr, false);
    } else if (newPressed)
    {
        SDL_ShowOpenFileDialog(importCallback,
                               nullptr,
                               window,
                               DialogFilters::modelFilters.data(),
                               DialogFilters::modelFilters.size(),
                               nullptr,
                               false);
    } else if (savePressed)
    {
        if (!ModelRenderer::GetModel().ValidateLodDistances())
        {
            if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                          "Invalid Model",
                                          "LOD distances are invalid! Please fix them in the LOD editor and make sure "
                                          "that:\n- The first LOD (LOD 0) has a distance of 0\n- No two LODs have the "
                                          "same distance",
                                          window))
            {
                printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
            }
        } else
        {
            SDL_ShowSaveFileDialog(saveGmdlCallback, nullptr, window, DialogFilters::gmdlFilters.data(), 1, nullptr);
        }
    }
}

static ImGuiID dockspaceId;
static ImGuiID rootDockspaceID;
static bool dockspaceSetup = false;

void SetupDockspace()
{
    if (dockspaceSetup)
    {
        return;
    }
    dockspaceSetup = true;
    dockspaceId = ImGui::GetID("dockspace");
    rootDockspaceID = dockspaceId;
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_NoCloseButton);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

    const ImGuiID rightDock = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Right, 0.4f, nullptr, &dockspaceId);

    ImGui::DockBuilderDockWindow("Model Preview", dockspaceId);
    ImGui::DockBuilderDockWindow("Preview Options", rightDock);
    ImGui::DockBuilderDockWindow("Collision", rightDock);
    ImGui::DockBuilderDockWindow("LODs", rightDock);
    ImGui::DockBuilderDockWindow("Materials", rightDock);
    ImGui::DockBuilderDockWindow("Skins", rightDock);

    ImGui::DockBuilderFinish(rootDockspaceID);
}

static void Render()
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();

    const ImVec2 VpAreaTopLeft = ImVec2(viewport->WorkPos.x, viewport->WorkPos.y);
    const ImVec2 VpAreaSize = ImVec2((viewport->WorkSize.x), (viewport->WorkSize.y));
    ImGui::SetNextWindowPos(VpAreaTopLeft);
    ImGui::SetNextWindowSize(VpAreaSize);

    if (!modelLoaded)
    {
        ImGui::Begin("CentralDock",
                     nullptr,
                     ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::TextDisabled("No model is open. Open or create one from the File menu.");
        ImGui::End();

        HandleMenuAndShortcuts();

        return;
    }

    ImGui::PushStyleVarX(ImGuiStyleVar_WindowPadding, 0.0f);
    ImGui::PushStyleVarY(ImGuiStyleVar_WindowPadding, 0.0f);
    ImGui::Begin("CentralDock",
                 nullptr,
                 ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::DockSpace(rootDockspaceID);
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    HandleMenuAndShortcuts();

    PreviewOptionsWindow::Render();
    MaterialEditWindow::Render();
    SkinEditWindow::Render();
    LodEditWindow::Render(window);
    CollisionEditWindow::Render(window);

    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                                             ImGuiWindowFlags_NoDecoration |
                                             ImGuiWindowFlags_NoScrollbar |
                                             ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::PushStyleVarX(ImGuiStyleVar_WindowPadding, 0.0f);
    ImGui::PushStyleVarY(ImGuiStyleVar_WindowPadding, 0.0f);
    if (ImGui::Begin("Model Preview", nullptr, windowFlags))
    {
        ImVec2 windowSize = ImGui::GetContentRegionMax();
        windowSize.x += 8;
        windowSize.y += 8;
        ModelRenderer::ResizeWindow(static_cast<GLsizei>(windowSize.x), static_cast<GLsizei>(windowSize.y));

        previewFocused = ImGui::IsWindowHovered();
        ImGui::Image(ModelRenderer::GetFramebufferTexture(), ModelRenderer::GetFramebufferSize(), {0, 1}, {1, 0});
    }
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (modelLoaded)
    {
        ModelRenderer::Render();
    }
}

int main(int argc, char **argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    SharedMgr::InitSharedMgr<OpenGLImGuiTextureAssetCache>();

    const char *glslVersion = "#version 130";
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0))
    {
        printf("Error: SDL_GL_SetAttribute(): %s\n", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE))
    {
        printf("Error: SDL_GL_SetAttribute(): %s\n", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3))
    {
        printf("Error: SDL_GL_SetAttribute(): %s\n", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3))
    {
        printf("Error: SDL_GL_SetAttribute(): %s\n", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1))
    {
        printf("Error: SDL_GL_SetAttribute(): %s\n", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24))
    {
        printf("Error: SDL_GL_SetAttribute(): %s\n", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0))
    {
        printf("Error: SDL_GL_SetAttribute(): %s\n", SDL_GetError());
    }

    constexpr SDL_WindowFlags sdlWindowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow("mdledit", 1366, 768, sdlWindowFlags);
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

    if (!SDL_GL_MakeCurrent(window, glContext))
    {
        printf("Error: SDL_GL_MakeCurrent(): %s\n", SDL_GetError());
        return -1;
    }
    if (!ModelRenderer::Init())
    {
        printf("Failed to start renderer!\n");
        return -1;
    }
    if (!SDL_GL_SetSwapInterval(1)) // Enable vsync
    {
        printf("Error: SDL_GL_SetSwapInterval(): %s\n", SDL_GetError());
    }
    if (!SDL_ShowWindow(window))
    {
        printf("Error: SDL_ShowWindow(): %s\n", SDL_GetError());
        return -1;
    }

    dynamic_cast<OpenGLImGuiTextureAssetCache *>(SharedMgr::textureCache.get())->InitMissingTexture();


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    SharedMgr::ApplyTheme();

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(glslVersion);

    const std::string &openPath = DesktopInterface::GetFileArgument(argc, argv, {".gmdl"});
    if (!openPath.empty())
    {
        SDL_Event event;
        event.type = ModelRenderer::EVENT_RELOAD_MODEL;
        event.user.code = ModelRenderer::EVENT_RELOAD_MODEL_CODE_GMDL;
        event.user.data1 = new std::string(openPath);
        if (!SDL_PushEvent(&event))
        {
            printf("Error: SDL_PushEvent(): %s\n", SDL_GetError());
        }
    } else
    {
        const std::string &importPath = DesktopInterface::GetFileArgument(argc,
                                                                          argv,
                                                                          {".obj", ".fbx", ".gltf", ".dae"});
        if (!importPath.empty())
        {
            SDL_Event event;
            event.type = ModelRenderer::EVENT_RELOAD_MODEL;
            event.user.code = ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_MODEL;
            event.user.data1 = new std::string(importPath);
            if (!SDL_PushEvent(&event))
            {
                printf("Error: SDL_PushEvent(): %s\n", SDL_GetError());
            }
        }
    }

    // ReSharper disable once CppDFALoopConditionNotUpdated Wrong again
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ProcessEvent(&event, io);
        }

        if ((SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0)
        {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        SetupDockspace();

        Render();

        SharedMgr::RenderSharedUI(window);
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (!SDL_GL_SwapWindow(window))
        {
            printf("Error: SDL_GL_SwapWindow(): %s\n", SDL_GetError());
        }
    }

    SharedMgr::DestroySharedMgr();

    ModelRenderer::Destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    if (!SDL_GL_DestroyContext(glContext))
    {
        printf("Error: SDL_GL_DestroyContext(): %s\n", SDL_GetError());
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    destroyExistingModel();
    return 0;
}
