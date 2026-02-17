#include <cstdio>
#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/Options.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <libassets/asset/ModelAsset.h>
#include <libassets/util/Error.h>
#include <string>
#include <utility>

#include "ModelEditor.h"
#include "tabs/CollisionTab.h"
#include "tabs/LodsTab.h"
#include "tabs/MaterialsTab.h"
#include "tabs/PreviewOptionsTab.h"
#include "tabs/SkinsTab.h"

static bool openPressed = false;
static bool newPressed = false;
static bool savePressed = false;

static ImGuiID dockspaceId;
static ImGuiID rootDockspaceID;
static bool dockspaceSetup = false;

static void openGmdl(const std::string &path)
{
    ModelAsset model;
    const Error::ErrorCode errorCode = ModelAsset::CreateFromAsset(path, model);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to open the model!\n{}", errorCode));
        return;
    }
    ModelEditor::DestroyExistingModel();
    ModelEditor::modelViewer.SetModel(std::move(model));
    ModelEditor::modelLoaded = true;
}

static void importModel(const std::string &path)
{
    ModelAsset model;
    const Error::ErrorCode errorCode = ModelAsset::CreateFromStandardModel(path,
                                                                           model,
                                                                           Options::Get().defaultTexture);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to import the model!\n{}", errorCode));
        return;
    }
    ModelEditor::DestroyExistingModel();
    ModelEditor::modelViewer.SetModel(std::move(model));
    ModelEditor::modelLoaded = true;
}

static void saveGmdl(const std::string &path)
{
    const Error::ErrorCode errorCode = ModelEditor::modelViewer.GetModel().SaveAsAsset(path);
    if (errorCode != Error::ErrorCode::OK)
    {
        SDKWindow::Get().ErrorMessage(std::format("Failed to save the model!\n{}", errorCode));
    }
}

static void HandleMenuAndShortcuts()
{
    newPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, ImGuiInputFlags_RouteGlobal);
    openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_RouteGlobal);
    savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal) && ModelEditor::modelLoaded;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            newPressed |= ImGui::MenuItem("New", "Ctrl+N");
            ImGui::Separator();
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S", false, ModelEditor::modelLoaded);
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                SDKWindow::Get().PostQuit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View", ModelEditor::modelLoaded))
        {
            if (ImGui::MenuItem("Reset View"))
            {
                ModelEditor::modelViewer.UpdateView(0, 0, 1);
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Display Mode"))
            {
                if (ImGui::MenuItem("Unshaded",
                                    "",
                                    ModelEditor::modelViewer.displayMode == ModelViewer::DisplayMode::COLORED))
                {
                    ModelEditor::modelViewer.displayMode = ModelViewer::DisplayMode::COLORED;
                }
                if (ImGui::MenuItem("Shaded",
                                    "",
                                    ModelEditor::modelViewer.displayMode == ModelViewer::DisplayMode::COLORED_SHADED))
                {
                    ModelEditor::modelViewer.displayMode = ModelViewer::DisplayMode::COLORED_SHADED;
                }
                if (ImGui::MenuItem("Textured Unshaded",
                                    "",
                                    ModelEditor::modelViewer.displayMode == ModelViewer::DisplayMode::TEXTURED))
                {
                    ModelEditor::modelViewer.displayMode = ModelViewer::DisplayMode::TEXTURED;
                }
                if (ImGui::MenuItem("Textured Shaded",
                                    "",
                                    ModelEditor::modelViewer.displayMode == ModelViewer::DisplayMode::TEXTURED_SHADED))
                {
                    ModelEditor::modelViewer.displayMode = ModelViewer::DisplayMode::TEXTURED_SHADED;
                }
                if (ImGui::MenuItem("UV Debug",
                                    "",
                                    ModelEditor::modelViewer.displayMode == ModelViewer::DisplayMode::UV))
                {
                    ModelEditor::modelViewer.displayMode = ModelViewer::DisplayMode::UV;
                }
                if (ImGui::MenuItem("Normal Debug",
                                    "",
                                    ModelEditor::modelViewer.displayMode == ModelViewer::DisplayMode::NORMAL))
                {
                    ModelEditor::modelViewer.displayMode = ModelViewer::DisplayMode::NORMAL;
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Show Backfaces", "", !ModelEditor::modelViewer.cullBackfaces))
            {
                ModelEditor::modelViewer.cullBackfaces = !ModelEditor::modelViewer.cullBackfaces;
            }
            if (ImGui::MenuItem("Show Unit Cube", "", ModelEditor::modelViewer.showUnitCube))
            {
                ModelEditor::modelViewer.showUnitCube = !ModelEditor::modelViewer.showUnitCube;
            }
            if (ImGui::MenuItem("Wireframe", "", ModelEditor::modelViewer.wireframe))
            {
                ModelEditor::modelViewer.wireframe = !ModelEditor::modelViewer.wireframe;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Show Bounding Box", "", ModelEditor::modelViewer.showBoundingBox))
            {
                ModelEditor::modelViewer.showBoundingBox = !ModelEditor::modelViewer.showBoundingBox;
            }
            if (ImGui::MenuItem("Show Collision Model", "", ModelEditor::modelViewer.showCollisionModel))
            {
                ModelEditor::modelViewer.showCollisionModel = !ModelEditor::modelViewer.showCollisionModel;
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
        if (!ModelEditor::modelViewer.GetModel().ValidateLodDistances())
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

    if (!ModelEditor::modelLoaded)
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
    ModelEditor::modelViewer.RenderWindow("Model Preview", windowFlags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

int main(int argc, char **argv)
{
    if (!SDKWindow::Get().Init("GAME SDK Model Editor", {1366, 768}))
    {
        return -1;
    }
    if (!ModelEditor::modelViewer.Init())
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

    ModelEditor::modelViewer.Destroy();
    SDKWindow::Get().Destroy();
    return 0;
}
