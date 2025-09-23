#include <cstdio>
#include <format>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <libassets/asset/LevelAsset.h>
#include <libassets/util/Error.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include "DialogFilters.h"
#include "imgui_internal.h"
#include "LevelEditor.h"
#include "LevelRenderer.h"
#include "OpenGLImGuiTextureAssetCache.h"
#include "SharedMgr.h"
#include "tools/AddPolygonTool.h"
#include "tools/AddPrimitiveTool.h"
#include "tools/EditorTool.h"
#include "Viewport.h"

static SDL_Window *window = nullptr;
static SDL_GLContext glContext = nullptr;

static Viewport vpTopDown = Viewport(ImVec2(0, 0), ImVec2(2, 1), Viewport::ViewportType::TOP_DOWN_XZ);
static Viewport vpFront = Viewport(ImVec2(0, 1), ImVec2(1, 1), Viewport::ViewportType::FRONT_XY);
static Viewport vpSide = Viewport(ImVec2(1, 1), ImVec2(1, 1), Viewport::ViewportType::SIDE_YZ);

static void Render(bool &done, SDL_Window *sdlWindow)
{
    if (ImGui::Shortcut(ImGuiKey_RightBracket, ImGuiInputFlags_RouteGlobal))
    {
        LevelEditor::gridSpacingIndex += 1;
        if (LevelEditor::gridSpacingIndex >= LevelEditor::GRID_SPACING_VALUES.size())
        {
            LevelEditor::gridSpacingIndex = LevelEditor::GRID_SPACING_VALUES.size() - 1;
        }
    } else if (ImGui::Shortcut(ImGuiKey_LeftBracket, ImGuiInputFlags_RouteGlobal))
    {
        LevelEditor::gridSpacingIndex -= 1;
        if (LevelEditor::gridSpacingIndex < 0)
        {
            LevelEditor::gridSpacingIndex = 0;
        }
    } else if (ImGui::Shortcut(ImGuiKey_Backslash, ImGuiInputFlags_RouteGlobal))
    {
        LevelEditor::gridSpacingIndex = 2;
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
        vpTopDown.GetZoom() = 20;
        vpFront.GetZoom() = 20;
        vpSide.GetZoom() = 20;
    }

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_R, ImGuiInputFlags_RouteGlobal))
    {
        vpTopDown.GetZoom() = 20;
        vpFront.GetZoom() = 20;
        vpSide.GetZoom() = 20;
        vpTopDown.CenterPosition(glm::vec3(0));
        vpFront.CenterPosition(glm::vec3(0));
        vpSide.CenterPosition(glm::vec3(0));
    }

    if (ImGui::Shortcut(ImGuiMod_Alt | ImGuiKey_V, ImGuiInputFlags_RouteGlobal))
    {
        vpTopDown.ToggleFullscreen();
    }

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("New TODO", "Ctrl+N");
            ImGui::Separator();
            ImGui::MenuItem("Open TODO", "Ctrl+O");
            ImGui::MenuItem("Save TODO", "Ctrl+S");
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
            {
                done = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::MenuItem("Selection Properties TODO", "Alt+Enter");
            ImGui::Separator();
            if (ImGui::MenuItem("Snap on Grid", "", LevelEditor::snapToGrid))
            {
                LevelEditor::snapToGrid = !LevelEditor::snapToGrid;
            }
            if (ImGui::MenuItem("Smaller Grid", "["))
            {
                LevelEditor::gridSpacingIndex -= 1;
                if (LevelEditor::gridSpacingIndex < 0)
                {
                    LevelEditor::gridSpacingIndex = 0;
                }
            }
            if (ImGui::MenuItem("Larger Grid", "]"))
            {
                LevelEditor::gridSpacingIndex += 1;
                if (LevelEditor::gridSpacingIndex >= LevelEditor::GRID_SPACING_VALUES.size())
                {
                    LevelEditor::gridSpacingIndex = LevelEditor::GRID_SPACING_VALUES.size() - 1;
                }
            }
            if (ImGui::MenuItem("Reset Grid", "\\"))
            {
                LevelEditor::gridSpacingIndex = 2;
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Reset All Views", "Ctrl+R"))
            {
                vpTopDown.GetZoom() = 20;
                vpFront.GetZoom() = 20;
                vpSide.GetZoom() = 20;
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
            ImGui::MenuItem("Center Selection TODO");
            ImGui::Separator();
            if (ImGui::MenuItem("Show Viewport Information", "", LevelEditor::drawViewportInfo))
            {
                LevelEditor::drawViewportInfo = !LevelEditor::drawViewportInfo;
            }
            if (ImGui::MenuItem("Show Axes", "", LevelEditor::drawAxisHelper))
            {
                LevelEditor::drawAxisHelper = !LevelEditor::drawAxisHelper;
            }
            if (ImGui::MenuItem("Show World Border", "", LevelEditor::drawWorldBorder))
            {
                LevelEditor::drawWorldBorder = !LevelEditor::drawWorldBorder;
            }
            if (ImGui::MenuItem("Show Grid", "", LevelEditor::drawGrid))
            {
                LevelEditor::drawGrid = !LevelEditor::drawGrid;
            }
            if (ImGui::MenuItem("Smaller Grid", "["))
            {
                LevelEditor::gridSpacingIndex -= 1;
                if (LevelEditor::gridSpacingIndex < 0)
                {
                    LevelEditor::gridSpacingIndex = 0;
                }
            }
            if (ImGui::MenuItem("Larger Grid", "]"))
            {
                LevelEditor::gridSpacingIndex += 1;
                if (LevelEditor::gridSpacingIndex >= LevelEditor::GRID_SPACING_VALUES.size())
                {
                    LevelEditor::gridSpacingIndex = LevelEditor::GRID_SPACING_VALUES.size() - 1;
                }
            }
            if (ImGui::MenuItem("Reset Grid", "\\"))
            {
                LevelEditor::gridSpacingIndex = 3;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Show Secondary Viewports", "Alt+V", !vpTopDown.IsFullscreen()))
            {
                vpTopDown.ToggleFullscreen();
            }
            if (ImGui::MenuItem("Show Sidebar", "", LevelEditor::showSidebar))
            {
                LevelEditor::showSidebar = !LevelEditor::showSidebar;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("Compile Map TODO", "F5");
            ImGui::MenuItem("Generate Benchmark TODO");
            ImGui::Separator();
            ImGui::EndMenu();
        }
        SharedMgr::SharedMenuUI("lvledit");
        ImGui::EndMainMenuBar();
    }

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImVec2 workSize{viewport->WorkSize.x, 32};
    const ImVec2 workPos{viewport->WorkPos.x, viewport->WorkPos.y};
    ImGui::SetNextWindowPos(workPos);
    ImGui::SetNextWindowSize(workSize);
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                                             ImGuiWindowFlags_NoDocking;
    ImGui::Begin("toolbar", nullptr, windowFlags);
    static int tool = static_cast<int>(LevelEditor::toolType);
    if (ImGui::RadioButton("Sector Editor", &tool, static_cast<int>(LevelEditor::EditorToolType::EDIT_SECTOR)))
    {
        LevelEditor::tool = std::unique_ptr<EditorTool>(new VertexTool());
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Add Primitive", &tool, static_cast<int>(LevelEditor::EditorToolType::ADD_PRIMITIVE)))
    {
        LevelEditor::tool = std::unique_ptr<EditorTool>(new AddPrimitiveTool());
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Add Polygon", &tool, static_cast<int>(LevelEditor::EditorToolType::ADD_POLYGON)))
    {
        LevelEditor::tool = std::unique_ptr<EditorTool>(new AddPolygonTool());
    }
    ImGui::SameLine();
    LevelEditor::toolType = static_cast<LevelEditor::EditorToolType>(tool);
    ImGui::End();

    if (LevelEditor::showSidebar)
    {
        ImGui::SetNextWindowPos(ImVec2(0, viewport->WorkPos.y + 32));
        ImGui::SetNextWindowSize(ImVec2(250, viewport->WorkSize.y - 32));
        ImGui::Begin("Tools",
                     nullptr,
                     ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoDecoration);
        LevelEditor::tool->RenderToolWindow();
        ImGui::End();
    }

    vpTopDown.RenderImGui();
    if (!vpTopDown.IsFullscreen())
    {
        vpFront.RenderImGui();
        vpSide.RenderImGui();
    }
}

int main()
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

    constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_HIDDEN |
                                            SDL_WINDOW_OPENGL |
                                            SDL_WINDOW_RESIZABLE |
                                            SDL_WINDOW_MAXIMIZED;
    window = SDL_CreateWindow("lvledit (beta)", 1366, 778, windowFlags);
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
    if (!LevelRenderer::Init())
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    SharedMgr::ApplyTheme();

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(glslVersion);

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

    LevelRenderer::Destroy();
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
