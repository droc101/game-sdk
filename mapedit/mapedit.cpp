#include <array>
#include <cassert>
#include <cstdio>
#include <format>
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/Options.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <libassets/util/Error.h>
#include <memory>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_video.h>
#include "ActorBrowserWindow.h"
#include "MapCompileWindow.h"
#include "MapEditor.h"
#include "MapPropertiesWindow.h"
#include "MapRenderer.h"
#include "tools/AddActorTool.h"
#include "tools/AddPolygonTool.h"
#include "tools/AddPrimitiveTool.h"
#include "tools/EditorTool.h"
#include "tools/SelectTool.h"
#include "Viewport.h"

static SDKWindow sdkWindow{};

static Viewport vpTopDown = Viewport(Viewport::ViewportType::TOP_DOWN_XZ);
static Viewport vpFront = Viewport(Viewport::ViewportType::FRONT_XY);
static Viewport vpSide = Viewport(Viewport::ViewportType::SIDE_YZ);

static ImGuiID dockspaceId;
static ImGuiID rootDockspaceID;
static bool dockspaceSetup = false;

static bool ToolbarToolButton(const char *id,
                              const char *tooltip,
                              const char *icon,
                              const bool selected,
                              const int spacing = 2,
                              const char *shortcutText = nullptr,
                              const ImGuiKeyChord shortcut = 0)
{
    const bool r = ImGui::Selectable(id, selected, 0, ImVec2(32, 32)) ||
                   (shortcutText != nullptr && ImGui::Shortcut(shortcut, ImGuiInputFlags_RouteGlobal));
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tooltip);
        if (shortcutText != nullptr)
        {
            ImGui::TextDisabled("%s", shortcutText);
        }
        ImGui::EndTooltip();
    }
    ImTextureID tex = 0;
    const Error::ErrorCode e = SharedMgr::textureCache.GetTextureID(icon, tex);
    assert(e == Error::ErrorCode::OK);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 40);
    ImGui::Image(tex, ImVec2(32, 32));
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + static_cast<float>(spacing));
    return r;
}

static void saveJsonCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = MapEditor::map.SaveAsMapSrc(fileList[0]);
    if (errorCode != Error::ErrorCode::OK)
    {
        sdkWindow.ErrorMessage(std::format("Failed to save the map!\n{}", errorCode));
        return;
    }
    MapEditor::mapFile = fileList[0];
}

static void openJson(const std::string &path)
{
    const Error::ErrorCode errorCode = MapAsset::CreateFromMapSrc(path.c_str(), MapEditor::map);
    if (errorCode != Error::ErrorCode::OK)
    {
        sdkWindow.ErrorMessage(std::format("Failed to open the map!\n{}", errorCode));
        return;
    }
    MapEditor::mapFile = path;
    for (Actor &actor: MapEditor::map.actors)
    {
        if (!SharedMgr::actorDefinitions.contains(actor.className))
        {
            sdkWindow.ErrorMessage(std::format("Failed to open the map because it contains an unknown actor "
                                               "class \"{}\"",
                                               actor.className));
            MapEditor::map = MapAsset();
            return;
        }

        actor.ApplyDefinition(SharedMgr::actorDefinitions.at(actor.className), false);
    }
}

static void openJsonCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    openJson(fileList[0]);
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

    ImGuiID lowerLeftDock = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Down, 0.5f, nullptr, &dockspaceId);
    const ImGuiID lowerRightDock = ImGui::DockBuilderSplitNode(lowerLeftDock,
                                                               ImGuiDir_Right,
                                                               0.5f,
                                                               nullptr,
                                                               &lowerLeftDock);

    ImGui::DockBuilderDockWindow("Top down (XZ)", dockspaceId);
    ImGui::DockBuilderDockWindow("Front (XY)", lowerLeftDock);
    ImGui::DockBuilderDockWindow("Side (YZ)", lowerRightDock);

    ImGui::DockBuilderFinish(rootDockspaceID);
}

static void Render(SDL_Window *sdlWindow)
{
    SetupDockspace();

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    if (ImGui::Shortcut(ImGuiKey_RightBracket, ImGuiInputFlags_RouteGlobal))
    {
        MapEditor::gridSpacingIndex += 1;
        if (static_cast<size_t>(MapEditor::gridSpacingIndex) >= MapEditor::GRID_SPACING_VALUES.size())
        {
            MapEditor::gridSpacingIndex = static_cast<int>(MapEditor::GRID_SPACING_VALUES.size() - 1);
        }
    } else if (ImGui::Shortcut(ImGuiKey_LeftBracket, ImGuiInputFlags_RouteGlobal))
    {
        MapEditor::gridSpacingIndex -= 1;
        if (MapEditor::gridSpacingIndex < 0)
        {
            MapEditor::gridSpacingIndex = 0;
        }
    } else if (ImGui::Shortcut(ImGuiKey_Backslash, ImGuiInputFlags_RouteGlobal))
    {
        MapEditor::gridSpacingIndex = MapEditor::DEFAULT_GRID_SPACING_INDEX;
    }

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus, ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_Repeat))
    {
        vpTopDown.GetZoom() += 5;
        vpFront.GetZoom() += 5;
        vpSide.GetZoom() += 5;
        vpTopDown.ClampZoom();
        vpFront.ClampZoom();
        vpSide.ClampZoom();
    } else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal, ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_Repeat))
    {
        vpTopDown.GetZoom() -= 5;
        vpFront.GetZoom() -= 5;
        vpSide.GetZoom() -= 5;
        vpTopDown.ClampZoom();
        vpFront.ClampZoom();
        vpSide.ClampZoom();
    } else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_0, ImGuiInputFlags_RouteGlobal))
    {
        vpTopDown.GetZoom() = MapEditor::DEFAULT_ZOOM;
        vpFront.GetZoom() = MapEditor::DEFAULT_ZOOM;
        vpSide.GetZoom() = MapEditor::DEFAULT_ZOOM;
    }

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_R, ImGuiInputFlags_RouteGlobal))
    {
        vpTopDown.GetZoom() = MapEditor::DEFAULT_ZOOM;
        vpFront.GetZoom() = MapEditor::DEFAULT_ZOOM;
        vpSide.GetZoom() = MapEditor::DEFAULT_ZOOM;
        vpTopDown.CenterPosition(glm::vec3(0));
        vpFront.CenterPosition(glm::vec3(0));
        vpSide.CenterPosition(glm::vec3(0));
    }

    bool canCutCopy = MapEditor::toolType == MapEditor::EditorToolType::SELECT;
    if (canCutCopy)
    {
        canCutCopy &= dynamic_cast<SelectTool *>(MapEditor::tool.get())->IsCopyableSelected();
    }
    bool canPaste = MapEditor::clipboard.has_value() && MapEditor::toolType == MapEditor::EditorToolType::SELECT;
    bool canCenterSelection = MapEditor::toolType == MapEditor::EditorToolType::SELECT;
    if (canCenterSelection)
    {
        canCenterSelection &= dynamic_cast<SelectTool *>(MapEditor::tool.get())->HasSelection();
    }

    bool newPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, ImGuiInputFlags_RouteGlobal);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_RouteGlobal);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, ImGuiInputFlags_RouteGlobal);
    bool compilePressed = ImGui::Shortcut(ImGuiKey_F5, ImGuiInputFlags_RouteGlobal);
    bool cutPressed = canCutCopy && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_X, ImGuiInputFlags_RouteGlobal);
    bool copyPressed = canCutCopy && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C, ImGuiInputFlags_RouteGlobal);
    bool pastePressed = canPaste && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_V, ImGuiInputFlags_RouteGlobal);

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            newPressed |= ImGui::MenuItem("New", "Ctrl+N");
            ImGui::Separator();
            openPressed |= ImGui::MenuItem("Open", "Ctrl+O");
            savePressed |= ImGui::MenuItem("Save", "Ctrl+S");
            ImGui::Separator();
            compilePressed |= ImGui::MenuItem("Compile", "F5");
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                sdkWindow.PostQuit();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Map Properties", ""))
            {
                MapPropertiesWindow::visible = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Snap on Grid", "", MapEditor::snapToGrid))
            {
                MapEditor::snapToGrid = !MapEditor::snapToGrid;
            }
            if (ImGui::MenuItem("Smaller Grid", "["))
            {
                MapEditor::gridSpacingIndex -= 1;
                if (MapEditor::gridSpacingIndex < 0)
                {
                    MapEditor::gridSpacingIndex = 0;
                }
            }
            if (ImGui::MenuItem("Larger Grid", "]"))
            {
                MapEditor::gridSpacingIndex += 1;
                if (static_cast<size_t>(MapEditor::gridSpacingIndex) >= MapEditor::GRID_SPACING_VALUES.size())
                {
                    MapEditor::gridSpacingIndex = MapEditor::GRID_SPACING_VALUES.size() - 1;
                }
            }
            if (ImGui::MenuItem("Reset Grid", "\\"))
            {
                MapEditor::gridSpacingIndex = MapEditor::DEFAULT_GRID_SPACING_INDEX;
            }
            ImGui::Separator();
            cutPressed |= ImGui::MenuItem("Cut", "Ctrl+X", false, canCutCopy);
            copyPressed |= ImGui::MenuItem("Copy", "Ctrl+C", false, canCutCopy);
            pastePressed |= ImGui::MenuItem("Paste", "Ctrl+V", false, canPaste);

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Reset All Views", "Ctrl+R"))
            {
                vpTopDown.GetZoom() = MapEditor::DEFAULT_ZOOM;
                vpFront.GetZoom() = MapEditor::DEFAULT_ZOOM;
                vpSide.GetZoom() = MapEditor::DEFAULT_ZOOM;
                vpTopDown.CenterPosition(glm::vec3(0));
                vpFront.CenterPosition(glm::vec3(0));
                vpSide.CenterPosition(glm::vec3(0));
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Zoom In", "Ctrl+="))
            {
                vpTopDown.GetZoom() -= 5;
                vpFront.GetZoom() -= 5;
                vpSide.GetZoom() -= 5;
                vpTopDown.ClampZoom();
                vpFront.ClampZoom();
                vpSide.ClampZoom();
            }
            if (ImGui::MenuItem("Zoom Out", "Ctrl+-"))
            {
                vpTopDown.GetZoom() += 5;
                vpFront.GetZoom() += 5;
                vpSide.GetZoom() += 5;
                vpTopDown.ClampZoom();
                vpFront.ClampZoom();
                vpSide.ClampZoom();
            }
            if (ImGui::MenuItem("Reset Zoom", "Ctrl+0"))
            {
                vpTopDown.GetZoom() = 20;
                vpFront.GetZoom() = 20;
                vpSide.GetZoom() = 20;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Center Origin", "Ctrl+Home"))
            {
                vpTopDown.CenterPosition(glm::vec3(0));
                vpFront.CenterPosition(glm::vec3(0));
                vpSide.CenterPosition(glm::vec3(0));
            }
            if (ImGui::MenuItem("Center Selection", "", false, canCenterSelection))
            {
                const glm::vec3 center = dynamic_cast<SelectTool *>(MapEditor::tool.get())->SelectionCenter();
                vpTopDown.CenterPosition(center);
                vpFront.CenterPosition(center);
                vpSide.CenterPosition(center);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Show Viewport Information", "", MapEditor::drawViewportInfo))
            {
                MapEditor::drawViewportInfo = !MapEditor::drawViewportInfo;
            }
            if (ImGui::MenuItem("Show Axes", "", MapEditor::drawAxisHelper))
            {
                MapEditor::drawAxisHelper = !MapEditor::drawAxisHelper;
            }
            if (ImGui::MenuItem("Show World Border", "", MapEditor::drawWorldBorder))
            {
                MapEditor::drawWorldBorder = !MapEditor::drawWorldBorder;
            }
            if (ImGui::MenuItem("Show Grid", "", MapEditor::drawGrid))
            {
                MapEditor::drawGrid = !MapEditor::drawGrid;
            }
            if (ImGui::MenuItem("Smaller Grid", "["))
            {
                MapEditor::gridSpacingIndex -= 1;
                if (MapEditor::gridSpacingIndex < 0)
                {
                    MapEditor::gridSpacingIndex = 0;
                }
            }
            if (ImGui::MenuItem("Larger Grid", "]"))
            {
                MapEditor::gridSpacingIndex += 1;
                if (static_cast<size_t>(MapEditor::gridSpacingIndex) >= MapEditor::GRID_SPACING_VALUES.size())
                {
                    MapEditor::gridSpacingIndex = static_cast<int>(MapEditor::GRID_SPACING_VALUES.size() - 1);
                }
            }
            if (ImGui::MenuItem("Reset Grid", "\\"))
            {
                MapEditor::gridSpacingIndex = 3;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Show Sidebar", "", MapEditor::showSidebar))
            {
                MapEditor::showSidebar = !MapEditor::showSidebar;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Actor Class Browser"))
            {
                ActorBrowserWindow::visible = true;
            }
            ImGui::Separator();
            ImGui::EndMenu();
        }
        SharedMgr::SharedMenuUI("mapedit");
        ImGui::EndMainMenuBar();
    }

    if (newPressed)
    {
        MapEditor::map = MapAsset();
        MapEditor::toolType = MapEditor::EditorToolType::SELECT;
        MapEditor::tool = std::unique_ptr<EditorTool>(new SelectTool());
        MapEditor::mapFile = "";
    }
    if (openPressed)
    {
        SDL_ShowOpenFileDialog(openJsonCallback,
                               nullptr,
                               sdlWindow,
                               DialogFilters::mapJsonFilters.data(),
                               1,
                               nullptr,
                               false);
        MapEditor::toolType = MapEditor::EditorToolType::SELECT;
        MapEditor::tool = std::unique_ptr<EditorTool>(new SelectTool());
    }
    if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveJsonCallback, nullptr, sdlWindow, DialogFilters::mapJsonFilters.data(), 1, nullptr);
    }
    if (compilePressed)
    {
        MapCompileWindow::Show();
    }
    if (cutPressed)
    {
        dynamic_cast<SelectTool *>(MapEditor::tool.get())->Cut();
    }
    if (copyPressed)
    {
        dynamic_cast<SelectTool *>(MapEditor::tool.get())->Copy();
    }
    if (pastePressed)
    {
        dynamic_cast<SelectTool *>(MapEditor::tool.get())->Paste();
    }

    const ImVec2 workSize{viewport->WorkSize.x, MapEditor::TOOLBAR_HEIGHT};
    const ImVec2 workPos{viewport->WorkPos.x, viewport->WorkPos.y};
    ImGui::SetNextWindowPos(workPos);
    ImGui::SetNextWindowSize(workSize);
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                                             ImGuiWindowFlags_NoDocking;
    ImGui::Begin("toolbar", nullptr, windowFlags);

    if (ToolbarToolButton("##selectTool",
                          "Select",
                          MapEditor::SELECT_ICON_NAME,
                          MapEditor::toolType == MapEditor::EditorToolType::SELECT,
                          6,
                          "Ctrl+1",
                          ImGuiMod_Ctrl | ImGuiKey_1))
    {
        MapEditor::toolType = MapEditor::EditorToolType::SELECT;
        MapEditor::tool = std::unique_ptr<EditorTool>(new SelectTool());
    }

    if (ToolbarToolButton("##actorTool",
                          "Add Actor",
                          MapEditor::ACTOR_ICON_NAME,
                          MapEditor::toolType == MapEditor::EditorToolType::ADD_ACTOR,
                          6,
                          "Ctrl+2",
                          ImGuiMod_Ctrl | ImGuiKey_2))
    {
        MapEditor::toolType = MapEditor::EditorToolType::ADD_ACTOR;
        MapEditor::tool = std::unique_ptr<EditorTool>(new AddActorTool());
    }

    if (ToolbarToolButton("##primTool",
                          "Add Primitive",
                          MapEditor::PRIMITIVE_ICON_NAME,
                          MapEditor::toolType == MapEditor::EditorToolType::ADD_PRIMITIVE,
                          2,
                          "Ctrl+3",
                          ImGuiMod_Ctrl | ImGuiKey_3))
    {
        MapEditor::toolType = MapEditor::EditorToolType::ADD_PRIMITIVE;
        MapEditor::tool = std::unique_ptr<EditorTool>(new AddPrimitiveTool());
    }

    if (ToolbarToolButton("##polyTool",
                          "Add Polygon",
                          MapEditor::POLYGON_ICON_NAME,
                          MapEditor::toolType == MapEditor::EditorToolType::ADD_POLYGON,
                          2,
                          "Ctrl+4",
                          ImGuiMod_Ctrl | ImGuiKey_4))
    {
        MapEditor::toolType = MapEditor::EditorToolType::ADD_POLYGON;
        MapEditor::tool = std::unique_ptr<EditorTool>(new AddPolygonTool());
    }

    ImGui::Dummy({1, 1});
    ImGui::End();

    if (MapEditor::showSidebar)
    {
        ImGui::SetNextWindowPos(ImVec2(0, viewport->WorkPos.y + MapEditor::TOOLBAR_HEIGHT));
        ImGui::SetNextWindowSize(ImVec2(MapEditor::SIDEBAR_WIDTH, viewport->WorkSize.y - MapEditor::TOOLBAR_HEIGHT));
        ImGui::Begin("Tools",
                     nullptr,
                     ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_NoDocking);

        MapEditor::tool->RenderToolWindow();

        ImGui::End();
    }

    const float sidebarSize = MapEditor::showSidebar ? MapEditor::SIDEBAR_WIDTH : 0;
    const ImVec2 VpAreaTopLeft = ImVec2(viewport->WorkPos.x + sidebarSize,
                                        viewport->WorkPos.y + MapEditor::TOOLBAR_HEIGHT);
    const ImVec2 VpAreaSize = ImVec2((viewport->WorkSize.x - sidebarSize),
                                     (viewport->WorkSize.y - MapEditor::TOOLBAR_HEIGHT));
    ImGui::PushStyleVarX(ImGuiStyleVar_WindowPadding, 0.0f);
    ImGui::PushStyleVarY(ImGuiStyleVar_WindowPadding, 0.0f);
    ImGui::SetNextWindowPos(VpAreaTopLeft);
    ImGui::SetNextWindowSize(VpAreaSize);
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

    vpTopDown.RenderImGui();
    vpFront.RenderImGui();
    vpSide.RenderImGui();

    ActorBrowserWindow::Render();
    MapPropertiesWindow::Render();
    MapCompileWindow::Render(sdlWindow);
}

int main(int argc, char **argv)
{
    if (!sdkWindow.Init("mapedit", {1366, 768}, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED))
    {
        return -1;
    }

    MapEditor::mat = WallMaterial(Options::defaultMaterial);
    if (!MapRenderer::Init())
    {
        printf("Failed to start renderer!\n");
        return -1;
    }
    SDL_SetWindowMinimumSize(sdkWindow.GetWindow(), 640, 480);

    SharedMgr::textureCache.RegisterPng("assets/mapedit/select.png", MapEditor::SELECT_ICON_NAME);
    SharedMgr::textureCache.RegisterPng("assets/mapedit/actors.png", MapEditor::ACTOR_ICON_NAME);
    SharedMgr::textureCache.RegisterPng("assets/mapedit/primitives.png", MapEditor::PRIMITIVE_ICON_NAME);
    SharedMgr::textureCache.RegisterPng("assets/mapedit/polygon.png", MapEditor::POLYGON_ICON_NAME);

    vpTopDown.GetZoom() = MapEditor::DEFAULT_ZOOM;
    vpFront.GetZoom() = MapEditor::DEFAULT_ZOOM;
    vpSide.GetZoom() = MapEditor::DEFAULT_ZOOM;

    const std::string &openPath = DesktopInterface::GetFileArgument(argc, argv, {".json"});
    if (!openPath.empty())
    {
        openJson(openPath);
    }

    sdkWindow.MainLoop(Render);

    MapRenderer::Destroy();
    sdkWindow.Destroy();
    return 0;
}
