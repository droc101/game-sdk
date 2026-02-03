#include <SDL3/SDL_error.h>
#include <array>
#include <cstdio>
#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/Options.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <GL/glew.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <libassets/asset/ModelAsset.h>
#include <libassets/type/ConvexHull.h>
#include <libassets/type/StaticCollisionMesh.h>
#include <libassets/util/Error.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <utility>
#include "ModelRenderer.h"
#include "tabs/CollisionTab.h"
#include "tabs/LodsTab.h"
#include "tabs/MaterialsTab.h"
#include "tabs/PreviewOptionsTab.h"
#include "tabs/SkinsTab.h"

static SDKWindow sdkWindow{};

static bool modelLoaded = false;
static bool dragging = false;

static bool openPressed = false;
static bool newPressed = false;
static bool savePressed = false;
static bool previewFocused = false;

static ImGuiID dockspaceId;
static ImGuiID rootDockspaceID;
static bool dockspaceSetup = false;

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
        sdkWindow.ErrorMessage(std::format("Failed to save the model!\n{}", errorCode));
    }
}

static bool ProcessEvent(SDL_Event *event)
{
    const ImGuiIO io = ImGui::GetIO();
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
                sdkWindow.ErrorMessage(std::format("Failed to open the model!\n{}", errorCode));
                delete path;
                return true;
            }
        } else if (event->user.code == ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_MODEL)
        {
            const Error::ErrorCode errorCode = ModelAsset::CreateFromStandardModel(*path,
                                                                                   model,
                                                                                   Options::defaultTexture);
            if (errorCode != Error::ErrorCode::OK)
            {
                sdkWindow.ErrorMessage(std::format("Failed to import the model!\n{}", errorCode));
                delete path;
                return true;
            }
        } else if (event->user.code == ModelRenderer::EVENT_RELOAD_MODEL_CODE_IMPORT_LOD)
        {
            model = ModelRenderer::GetModel();
            if (!model.AddLod(*path))
            {
                sdkWindow.ErrorMessage(std::format("Failed to import model LOD!"));
                delete path;
                return true;
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
        return true;
    }
    if (event->type == ModelRenderer::EVENT_SAVE_MODEL)
    {
        const std::string *path = static_cast<std::string *>(event->user.data1);
        const Error::ErrorCode errorCode = ModelRenderer::GetModel().SaveAsAsset(*path);
        if (errorCode != Error::ErrorCode::OK)
        {
            sdkWindow.ErrorMessage(std::format("Failed to save the model!\n{}", errorCode));
            delete path;
            return true;
        }
        delete path;
        return true;
    }
    if (previewFocused)
    {
        if (event->type == SDL_EVENT_MOUSE_WHEEL)
        {
            const SDL_MouseWheelEvent &mouseWheelEvent = event->wheel;
            ModelRenderer::UpdateViewRel(0, 0, mouseWheelEvent.y / -10.0f);
            return true;
        }
        if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event->button.button == SDL_BUTTON_LEFT)
            {
                dragging = false;
                return true;
            }
        } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            if (event->button.button == SDL_BUTTON_LEFT)
            {
                dragging = true;
                return true;
            }
        }
        if (dragging && event->type == SDL_EVENT_MOUSE_MOTION)
        {
            ModelRenderer::UpdateViewRel(event->motion.yrel / 5.0f, event->motion.xrel / -5.0f, 0);
            return true;
        }
    } else
    {
        dragging = false;
    }
    return false;
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
                sdkWindow.PostQuit();
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
        SDL_ShowOpenFileDialog(openGmdlCallback,
                               nullptr,
                               sdkWindow.GetWindow(),
                               DialogFilters::gmdlFilters.data(),
                               1,
                               nullptr,
                               false);
    } else if (newPressed)
    {
        SDL_ShowOpenFileDialog(importCallback,
                               nullptr,
                               sdkWindow.GetWindow(),
                               DialogFilters::modelFilters.data(),
                               DialogFilters::modelFilters.size(),
                               nullptr,
                               false);
    } else if (savePressed)
    {
        if (!ModelRenderer::GetModel().ValidateLodDistances())
        {
            sdkWindow.ErrorMessage("LOD distances are invalid! Please fix them in the LOD editor and make sure "
                                   "that:\n- The first LOD (LOD 0) has a distance of 0\n- No two LODs have the "
                                   "same distance",
                                   "Invalid Model");
        } else
        {
            SDL_ShowSaveFileDialog(saveGmdlCallback,
                                   nullptr,
                                   sdkWindow.GetWindow(),
                                   DialogFilters::gmdlFilters.data(),
                                   1,
                                   nullptr);
        }
    }
}

static void SetupDockspace()
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

static void Render(SDL_Window *window)
{
    SetupDockspace();

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

    PreviewOptionsTab::Render();
    MaterialsTab::Render();
    SkinsTab::Render();
    LodsTab::Render(window);
    CollisionTab::Render(window);

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
    if (!sdkWindow.Init("mdledit", {1366, 768}))
    {
        return -1;
    }

    ModelRenderer::EVENT_RELOAD_MODEL = SDL_RegisterEvents(2);
    ModelRenderer::EVENT_SAVE_MODEL = ModelRenderer::EVENT_RELOAD_MODEL + 1;
    if (!ModelRenderer::Init())
    {
        printf("Failed to start renderer!\n");
        return -1;
    }

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

    sdkWindow.MainLoop(Render, ProcessEvent);

    destroyExistingModel();
    ModelRenderer::Destroy();
    sdkWindow.Destroy();
    return 0;
}
