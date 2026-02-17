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
#include <string>
#include <utility>
#include "ModelRenderer.h"
#include "tabs/CollisionTab.h"
#include "tabs/LodsTab.h"
#include "tabs/MaterialsTab.h"
#include "tabs/PreviewOptionsTab.h"
#include "tabs/SkinsTab.h"

static bool modelLoaded = false;

static bool openPressed = false;
static bool newPressed = false;
static bool savePressed = false;

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

static void openGmdl(const std::string &path)
{
    ModelAsset model;
    const Error::ErrorCode errorCode = ModelAsset::CreateFromAsset(path.c_str(), model);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to open the model!\n{}", errorCode));
        return;
    }
    destroyExistingModel();
    ModelRenderer::LoadModel(std::move(model));
    modelLoaded = true;
}

static void importModel(const std::string &path)
{
    ModelAsset model;
    const Error::ErrorCode errorCode = ModelAsset::CreateFromStandardModel(path.c_str(),
                                                                           model,
                                                                           Options::Get().defaultTexture);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to import the model!\n{}", errorCode));
        return;
    }
    destroyExistingModel();
    ModelRenderer::LoadModel(std::move(model));
    modelLoaded = true;
}

static void saveGmdl(const std::string &path)
{
    const Error::ErrorCode errorCode = ModelRenderer::GetModel().SaveAsAsset(path);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to save the model!\n{}", errorCode));
    }
}

#pragma region functions that CANNOT BE STATIC AND ARE USED DO NOT BELEIVE THE LIES

void importLod(const std::string &path)
{
    ModelAsset model = ModelRenderer::GetModel();
    if (!model.AddLod(path))
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to import model LOD!"));
        return;
    }
    destroyExistingModel();
    ModelRenderer::LoadModel(std::move(model));
    modelLoaded = true;
}

void importSingleHull(const std::string &path)
{
    ModelAsset model = ModelRenderer::GetModel();
    const ConvexHull hull = ConvexHull(path);
    model.AddHull(hull);
    destroyExistingModel();
    ModelRenderer::LoadModel(std::move(model));
    modelLoaded = true;
}

void importMultipleHulls(const std::string &path)
{
    ModelAsset model = ModelRenderer::GetModel();
    model.AddHulls(path);
    destroyExistingModel();
    ModelRenderer::LoadModel(std::move(model));
    modelLoaded = true;
}

void importStaticCollider(const std::string &path)
{
    ModelAsset model = ModelRenderer::GetModel();
    model.SetStaticCollisionMesh(StaticCollisionMesh(path));
    destroyExistingModel();
    ModelRenderer::LoadModel(std::move(model));
    modelLoaded = true;
}

#pragma endregion

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
                SDKWindow::Get().PostQuit();
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
        SharedMgr::Get().SharedMenuUI("mdledit");
        ImGui::EndMainMenuBar();
    }

    if (openPressed)
    {
        SDKWindow::Get().OpenFileDialog(openGmdl, DialogFilters::gmdlFilters);
    } else if (newPressed)
    {
        SDKWindow::Get().OpenFileDialog(importModel, DialogFilters::modelFilters);
    } else if (savePressed)
    {
        if (!ModelRenderer::GetModel().ValidateLodDistances())
        {
            SDKWindow::Get().ErrorMessage("LOD distances are invalid! Please fix them in the LOD editor and make sure "
                                          "that:\n- The first LOD (LOD 0) has a distance of 0\n- No two LODs have the "
                                          "same distance",
                                          "Invalid Model");
        } else
        {
            SDKWindow::Get().SaveFileDialog(saveGmdl, DialogFilters::gmdlFilters);
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

static void Render()
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
    LodsTab::Render();
    CollisionTab::Render();

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

        const bool previewFocused = ImGui::IsWindowHovered();
        ImGui::Image(ModelRenderer::GetFramebufferTexture(), ModelRenderer::GetFramebufferSize(), {0, 1}, {1, 0});

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            const ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            ModelRenderer::UpdateViewRel(dragDelta.y / 5.0f, dragDelta.x / -5.0f, 0);
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }

        const float mouseWheel = ImGui::GetIO().MouseWheel;
        if (mouseWheel != 0 && previewFocused)
        {
            ModelRenderer::UpdateViewRel(0, 0, mouseWheel / -10.0f);
        }
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
    if (!SDKWindow::Get().Init("GAME SDK Model Editor", {1366, 768}))
    {
        return -1;
    }
    if (!ModelRenderer::Init())
    {
        printf("Failed to start renderer!\n");
        return -1;
    }

    const std::string &openPath = DesktopInterface::Get().GetFileArgument(argc, argv, {".gmdl"});
    if (!openPath.empty())
    {
        openGmdl(openPath);
    } else
    {
        const std::string &importPath = DesktopInterface::Get().GetFileArgument(argc,
                                                                                argv,
                                                                                {".obj", ".fbx", ".gltf", ".dae"});
        if (!importPath.empty())
        {
            importModel(importPath);
        }
    }

    SDKWindow::Get().MainLoop(Render);

    destroyExistingModel();
    ModelRenderer::Destroy();
    SDKWindow::Get().Destroy();
    return 0;
}
