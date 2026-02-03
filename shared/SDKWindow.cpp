//
// Created by droc101 on 2/2/26.
//

#include "SDKWindow.h"
#include <cstdio>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <string>
#include "GLHelper.h"
#include "imgui.h"
#include "SharedMgr.h"

bool SDKWindow::Init(const std::string &appName, const glm::ivec2 windowSize, const SDL_WindowFlags windowFlags)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return false;
    }

    SharedMgr::InitSharedMgr();

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
    if (!SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0))
    {
        printf("Error: SDL_GL_SetAttribute(): %s\n", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0))
    {
        printf("Error: SDL_GL_SetAttribute(): %s\n", SDL_GetError());
    }

    const SDL_WindowFlags sdlWindowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | windowFlags;
    window = SDL_CreateWindow(appName.c_str(), windowSize.x, windowSize.y, sdlWindowFlags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return false;
    }
    glContext = SDL_GL_CreateContext(window);
    if (glContext == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return false;
    }

    if (!SDL_GL_MakeCurrent(window, glContext))
    {
        printf("Error: SDL_GL_MakeCurrent(): %s\n", SDL_GetError());
        return false;
    }

    if (!GLHelper::Init())
    {
        return false;
    }

    if (!SDL_GL_SetSwapInterval(1)) // Enable vsync
    {
        printf("Error: SDL_GL_SetSwapInterval(): %s\n", SDL_GetError());
    }

    SharedMgr::textureCache.InitMissingTexture();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    SharedMgr::ApplyTheme();

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(glslVersion);

    if (!SDL_ShowWindow(window))
    {
        printf("Error: SDL_ShowWindow(): %s\n", SDL_GetError());
        return false;
    }

    initDone = true;
    return true;
}

void SDKWindow::MainLoop(const SDKWindowRenderFunction Render, const SDKWindowProcessEventFunction ProcessEvent) const
{
    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                done = true;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
            {
                done = true;
            } else if (ProcessEvent == nullptr || !ProcessEvent(&event))
            {
                ImGui_ImplSDL3_ProcessEvent(&event);
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

        Render(window);

        SharedMgr::RenderSharedUI(window);
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (!SDL_GL_SwapWindow(window))
        {
            printf("Error: SDL_GL_SwapWindow(): %s\n", SDL_GetError());
        }

        if (quitRequest)
        {
            done = true;
        }
    }
}

SDL_Window *SDKWindow::GetWindow()
{
    return window;
}

void SDKWindow::PostQuit()
{
    quitRequest = true;
}

SDKWindow::~SDKWindow()
{
    if (initDone)
    {
        SharedMgr::DestroySharedMgr();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        if (!SDL_GL_DestroyContext(glContext))
        {
            printf("Error: SDL_GL_DestroyContext(): %s\n", SDL_GetError());
        }
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
}
