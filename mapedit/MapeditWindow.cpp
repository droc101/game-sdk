//
// Created by droc101 on 8/21/26.
//

#include "MapeditWindow.h"
#include <game_sdk/DesktopInterface.h>
#include <game_sdk/DialogFilters.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/SoundSystem.h>
#include <game_sdk/WindowManager.h>
#include <game_sdk/windows/MaterialBrowserWindow.h>
#include <game_sdk/windows/ModelBrowserWindow.h>
#include <game_sdk/windows/SoundBrowserWindow.h>
#include <game_sdk/windows/TextureBrowserWindow.h>
#include <imgui_internal.h>
#include "ActorBrowserWindow.h"
#include "game_sdk/Options.h"
#include "MapCompileWindow.h"
#include "MapEditor.h"
#include "MapPropertiesWindow.h"
#include "MapRenderer.h"
#include "tools/AddActorTool.h"
#include "tools/AddPolygonTool.h"
#include "tools/AddPrimitiveTool.h"

const Window::WindowProperties &MapeditWindow::GetProperties() const
{
    return properties;
}

bool MapeditWindow::Init()
{
    SoundSystem::Get().Init();

    Error::ErrorCode adms = Error::ErrorCode::UNKNOWN;
    MapEditor::adm = ActorDefinitionManager(SharedMgr::Get().pathManager, adms);
    if (adms != Error::ErrorCode::OK)
    {
        ErrorMessage("Failed to load actor definitions");
        return 1;
    }

    if (!MapEditor::adm.HasActorClass("player"))
    {
        ErrorMessage("Could not find definition for required actor class \"player\". Please check the "
                     "game paths from the SDK launcher.",
                     "Error");
        return false;
    }
    if (!MapEditor::adm.HasActorClass("actor"))
    {
        ErrorMessage("Could not find definition for required actor class \"actor\". Please check the "
                     "game paths from the SDK launcher.",
                     "Error");
        return false;
    }

    MapEditor::mat = WallMaterial(Options::Get().defaultMaterial);
    if (!MapRenderer::Init())
    {
        Logger::Error("Failed to start renderer!");
        return false;
    }
    (void)SDL_SetWindowMinimumSize(GetWindow(), 640, 480);

    (void)SharedMgr::Get().textureCache.RegisterPng("assets/icons/select.png", MapEditor::SELECT_ICON_NAME);
    (void)SharedMgr::Get().textureCache.RegisterPng("assets/icons/actors.png", MapEditor::ACTOR_ICON_NAME);
    (void)SharedMgr::Get().textureCache.RegisterPng("assets/icons/primitives.png", MapEditor::PRIMITIVE_ICON_NAME);
    (void)SharedMgr::Get().textureCache.RegisterPng("assets/icons/polygon.png", MapEditor::POLYGON_ICON_NAME);

    vpTopDown.GetZoom() = MapEditor::DEFAULT_ZOOM;
    vpFront.GetZoom() = MapEditor::DEFAULT_ZOOM;
    vpSide.GetZoom() = MapEditor::DEFAULT_ZOOM;

    const std::string &openPath = WindowManager::Get().GetArgumentParser().GetFileArgument({".json"});
    if (!openPath.empty())
    {
        OpenJson(openPath);
    }

    return true;
}

void MapeditWindow::Destroy()
{
    SoundSystem::Get().Destroy();
    MapRenderer::Destroy();
}

bool MapeditWindow::ToolbarToolButton(const char *id,
                                      const char *tooltip,
                                      const char *icon,
                                      const bool selected,
                                      const int spacing,
                                      const char *shortcutText,
                                      const ImGuiKeyChord shortcut)
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
    const Error::ErrorCode e = SharedMgr::Get().textureCache.GetTextureID(icon, tex);
    if (e != Error::ErrorCode::OK)
    {
        tex = SharedMgr::Get().textureCache.GetMissingTextureID();
    }
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 40);
    ImGui::Image(tex, ImVec2(32, 32));
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + static_cast<float>(spacing));
    return r;
}

void MapeditWindow::SetupDockspace()
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

    ImGuiID lowerLeftDock = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Down, 0.35f, nullptr, &dockspaceId);
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

void MapeditWindow::OpenJson(const std::string &path) const
{
    const Error::ErrorCode errorCode = MapEditor::map.Import(path);
    if (errorCode != Error::ErrorCode::OK)
    {
        ErrorMessage(std::format("Failed to open the map!\n{}", errorCode));
        return;
    }
    MapEditor::mapFile = path;
    for (Actor &actor: MapEditor::map.actors)
    {
        if (!MapEditor::adm.HasActorClass(actor.className))
        {
            ErrorMessage(std::format("Failed to open the map because it contains an unknown actor "
                                     "class \"{}\"",
                                     actor.className));
            MapEditor::map = MapAsset();
            return;
        }

        actor.ApplyDefinition(MapEditor::adm.GetActorDefinition(actor.className), false);
    }
}

void MapeditWindow::SaveJson(const std::string &path) const
{
    const Error::ErrorCode errorCode = MapEditor::map.Export(path);
    if (errorCode != Error::ErrorCode::OK)
    {
        ErrorMessage(std::format("Failed to save the map!\n{}", errorCode));
        return;
    }
    MapEditor::mapFile = path;
}

void MapeditWindow::Render()
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
        vpTopDown.ChangeZoom(Viewport::ZOOM_STEP);
        vpFront.ChangeZoom(Viewport::ZOOM_STEP);
        vpSide.ChangeZoom(Viewport::ZOOM_STEP);
    } else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal, ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_Repeat))
    {
        vpTopDown.ChangeZoom(-Viewport::ZOOM_STEP);
        vpFront.ChangeZoom(-Viewport::ZOOM_STEP);
        vpSide.ChangeZoom(-Viewport::ZOOM_STEP);
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
    const bool canPaste = MapEditor::clipboard.has_value() && MapEditor::toolType == MapEditor::EditorToolType::SELECT;
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
                RequestClose();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Map Properties", ""))
            {
                WindowManager::Get().AddModalWindow<MapPropertiesWindow>();
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
                vpTopDown.ChangeZoom(-Viewport::ZOOM_STEP);
                vpFront.ChangeZoom(-Viewport::ZOOM_STEP);
                vpSide.ChangeZoom(-Viewport::ZOOM_STEP);
            }
            if (ImGui::MenuItem("Zoom Out", "Ctrl+-"))
            {
                vpTopDown.ChangeZoom(Viewport::ZOOM_STEP);
                vpFront.ChangeZoom(Viewport::ZOOM_STEP);
                vpSide.ChangeZoom(Viewport::ZOOM_STEP);
            }
            if (ImGui::MenuItem("Reset Zoom", "Ctrl+0"))
            {
                vpTopDown.GetZoom() = MapEditor::DEFAULT_ZOOM;
                vpFront.GetZoom() = MapEditor::DEFAULT_ZOOM;
                vpSide.GetZoom() = MapEditor::DEFAULT_ZOOM;
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
            if (ImGui::MenuItem("Show 3D Models", "", MapEditor::drawModels))
            {
                MapEditor::drawModels = !MapEditor::drawModels;
            }
            if (ImGui::MenuItem("Show Gizmos", "", MapEditor::drawGizmos))
            {
                MapEditor::drawGizmos = !MapEditor::drawGizmos;
            }
            if (ImGui::MenuItem("Enable Culling", "", MapEditor::culling))
            {
                MapEditor::culling = !MapEditor::culling;
            }
            ImGui::Separator();
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
                MapEditor::gridSpacingIndex = MapEditor::DEFAULT_GRID_SPACING_INDEX;
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
                WindowManager::Get().AddModalWindow<ActorBrowserWindow>();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Texture Browser"))
            {
                TextureBrowserWindow::Show(&textureBrowserToolSelection);
            }
            if (ImGui::MenuItem("Material Browser"))
            {
                MaterialBrowserWindow::Show(&materialBrowserToolSelection);
            }
            if (ImGui::MenuItem("Model Browser"))
            {
                ModelBrowserWindow::Show(&modelBrowserToolSelection);
            }
            if (ImGui::MenuItem("Sound Browser"))
            {
                SoundBrowserWindow::Show(&soundBrowserToolSelection);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Remove unknown/obsolete actor params"))
            {
                for (Actor &actor: MapEditor::map.actors)
                {
                    const ActorDefinition &def = MapEditor::adm.GetActorDefinition(actor.className);
                    actor.RemoveUnknownParams(def);
                }
            }
            ImGui::Separator();
            ImGui::EndMenu();
        }
        SharedMgr::Get().SharedMenuUI("mapedit");
        ImGui::EndMainMenuBar();
    }

    if (newPressed)
    {
        MapEditor::map = MapAsset();
        MapEditor::toolType = MapEditor::EditorToolType::SELECT;
        MapEditor::tool = std::make_unique<SelectTool>();
        MapEditor::mapFile = "";
    }
    if (openPressed)
    {
        OpenFileDialog(FILE_DIALOG_CALLBACK(OpenJson), DialogFilters::MAP_JSON_FILTERS);
        MapEditor::toolType = MapEditor::EditorToolType::SELECT;
        MapEditor::tool = std::make_unique<SelectTool>();
    }
    if (savePressed)
    {
        SaveFileDialog(FILE_DIALOG_CALLBACK(SaveJson), DialogFilters::MAP_JSON_FILTERS);
    }
    if (compilePressed)
    {
        WindowManager::Get().AddModalWindow<MapCompileWindow>();
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
    constexpr ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoDecoration |
                                              ImGuiWindowFlags_NoMove |
                                              ImGuiWindowFlags_NoSavedSettings |
                                              ImGuiWindowFlags_NoBringToFrontOnFocus |
                                              ImGuiWindowFlags_NoDocking;
    ImGui::Begin("toolbar", nullptr, WINDOW_FLAGS);

    if (ToolbarToolButton("##selectTool",
                          "Select",
                          MapEditor::SELECT_ICON_NAME,
                          MapEditor::toolType == MapEditor::EditorToolType::SELECT,
                          6,
                          "Ctrl+1",
                          ImGuiMod_Ctrl | ImGuiKey_1))
    {
        MapEditor::toolType = MapEditor::EditorToolType::SELECT;
        MapEditor::tool = std::make_unique<SelectTool>();
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
        MapEditor::tool = std::make_unique<AddActorTool>();
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
        MapEditor::tool = std::make_unique<AddPrimitiveTool>();
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
        MapEditor::tool = std::make_unique<AddPolygonTool>();
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
    const ImVec2 vpAreaTopLeft = ImVec2(viewport->WorkPos.x + sidebarSize,
                                        viewport->WorkPos.y + MapEditor::TOOLBAR_HEIGHT);
    const ImVec2 vpAreaSize = ImVec2((viewport->WorkSize.x - sidebarSize),
                                     (viewport->WorkSize.y - MapEditor::TOOLBAR_HEIGHT));
    ImGui::PushStyleVarX(ImGuiStyleVar_WindowPadding, 0.0f);
    ImGui::PushStyleVarY(ImGuiStyleVar_WindowPadding, 0.0f);
    ImGui::SetNextWindowPos(vpAreaTopLeft);
    ImGui::SetNextWindowSize(vpAreaSize);
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

    vpTopDown.Render();
    vpFront.Render();
    vpSide.Render();
}
