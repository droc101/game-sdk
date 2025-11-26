#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <libassets/util/Error.h>
#include <memory>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_process.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include "ActorBrowserWindow.h"
#include "DialogFilters.h"
#include "MapCompileWindow.h"
#include "MapEditor.h"
#include "MapPropertiesWindow.h"
#include "MapRenderer.h"
#include "OpenGLImGuiTextureAssetCache.h"
#include "Options.h"
#include "SharedMgr.h"
#include "TextureBrowserWindow.h"
#include "tools/AddActorTool.h"
#include "tools/AddPolygonTool.h"
#include "tools/AddPrimitiveTool.h"
#include "tools/EditorTool.h"
#include "tools/SelectTool.h"
#include "Viewport.h"

static SDL_Window *window = nullptr;
static SDL_GLContext glContext = nullptr;

static Viewport vpTopDown = Viewport(ImVec2(0, 0), ImVec2(2, 1), Viewport::ViewportType::TOP_DOWN_XZ);
static Viewport vpFront = Viewport(ImVec2(0, 1), ImVec2(1, 1), Viewport::ViewportType::FRONT_XY);
static Viewport vpSide = Viewport(ImVec2(1, 1), ImVec2(1, 1), Viewport::ViewportType::SIDE_YZ);

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
    const Error::ErrorCode e = SharedMgr::textureCache->GetTextureID(icon, tex);
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
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to save the level!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
        return;
    }
    MapEditor::mapFile = fileList[0];
}

static void openJsonCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }
    const Error::ErrorCode errorCode = MapAsset::CreateFromMapSrc(fileList[0], MapEditor::map);
    if (errorCode != Error::ErrorCode::OK)
    {
        if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                      "Error",
                                      std::format("Failed to open the level!\n{}", errorCode).c_str(),
                                      window))
        {
            printf("Error: SDL_ShowSimpleMessageBox(): %s\n", SDL_GetError());
        }
        return;
    }
    MapEditor::mapFile = fileList[0];
}

static void Render(bool &done, SDL_Window *sdlWindow)
{
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

    if (ImGui::Shortcut(ImGuiMod_Alt | ImGuiKey_V, ImGuiInputFlags_RouteGlobal))
    {
        vpTopDown.ToggleFullscreen();
    }

    bool newPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, ImGuiInputFlags_RouteGlobal);
    bool openPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_RouteGlobal);
    bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, ImGuiInputFlags_RouteGlobal);
    bool compilePressed = ImGui::Shortcut(ImGuiKey_F5, ImGuiInputFlags_RouteGlobal);

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
                done = true;
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
            // ImGui::MenuItem("Center Selection TODO");
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
            if (ImGui::MenuItem("Show Secondary Viewports", "Alt+V", !vpTopDown.IsFullscreen()))
            {
                vpTopDown.ToggleFullscreen();
            }
            if (ImGui::MenuItem("Show Sidebar", "", MapEditor::showSidebar))
            {
                MapEditor::showSidebar = !MapEditor::showSidebar;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            // ImGui::MenuItem("Generate Benchmark TODO");
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
                                       DialogFilters::jsonFilters.data(),
                                       1,
                                       nullptr,
                                       false);
        MapEditor::toolType = MapEditor::EditorToolType::SELECT;
        MapEditor::tool = std::unique_ptr<EditorTool>(new SelectTool());
    }
    if (savePressed)
    {
        SDL_ShowSaveFileDialog(saveJsonCallback,
                                       nullptr,
                                       sdlWindow,
                                       DialogFilters::jsonFilters.data(),
                                       1,
                                       nullptr);
    }
    if (compilePressed)
    {
        MapCompileWindow::Show();
    }

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
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
                             ImGuiWindowFlags_NoDecoration);

        MapEditor::tool->RenderToolWindow();

        ImGui::End();
    }

    vpTopDown.RenderImGui();
    if (!vpTopDown.IsFullscreen())
    {
        vpFront.RenderImGui();
        vpSide.RenderImGui();
    }

    ActorBrowserWindow::Render();
    MapPropertiesWindow::Render();
    MapCompileWindow::Render(sdlWindow);
}

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    SharedMgr::InitSharedMgr<OpenGLImGuiTextureAssetCache>();
    MapEditor::mat = WallMaterial(Options::defaultMaterial);

    // Actor a = Actor();
    // a.className = "logic_counter";
    // a.ApplyDefinition(SharedMgr::actorDefinitions.at(a.className));
    // a.params.at("name").Set<std::string>("counter");
    // MapEditor::level.actors.push_back(a);

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

    constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_HIDDEN |
                                            SDL_WINDOW_OPENGL |
                                            SDL_WINDOW_RESIZABLE |
                                            SDL_WINDOW_MAXIMIZED;
    window = SDL_CreateWindow("mapedit", 1366, 778, windowFlags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetWindowMinimumSize(window, 640, 480);
    glContext = SDL_GL_CreateContext(window);
    if (glContext == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }

    if (!SDL_GL_MakeCurrent(window, glContext))
    {
        printf("Error: SDL_GL_MakeCurrent(): %s\n", SDL_GetError());
        return -1;
    }
    if (!MapRenderer::Init())
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

    SharedMgr::ApplyTheme();

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(glslVersion);

    SharedMgr::textureCache->RegisterPng("assets/mapedit/select.png", MapEditor::SELECT_ICON_NAME);
    SharedMgr::textureCache->RegisterPng("assets/mapedit/actors.png", MapEditor::ACTOR_ICON_NAME);
    SharedMgr::textureCache->RegisterPng("assets/mapedit/primitives.png", MapEditor::PRIMITIVE_ICON_NAME);
    SharedMgr::textureCache->RegisterPng("assets/mapedit/polygon.png", MapEditor::POLYGON_ICON_NAME);

    vpTopDown.GetZoom() = MapEditor::DEFAULT_ZOOM;
    vpFront.GetZoom() = MapEditor::DEFAULT_ZOOM;
    vpSide.GetZoom() = MapEditor::DEFAULT_ZOOM;

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

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        Render(done, window);

        SharedMgr::RenderSharedUI(window);

        ImGui::Render();
        // glClearColor(0, 0, 0, 1);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (!SDL_GL_SwapWindow(window))
        {
            printf("Error: SDL_GL_SwapWindow(): %s\n", SDL_GetError());
        }
    }

    SharedMgr::DestroySharedMgr();

    MapRenderer::Destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    if (!SDL_GL_DestroyContext(glContext))
    {
        printf("Error: SDL_GL_DestroyContext(): %s\n", SDL_GetError());
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
