//
// Created by droc101 on 8/20/26.
//

#include <cstdlib>
#include <game_sdk/gl/GLHelper.h>
#include <game_sdk/ModelViewer.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/Window.h>
#include <game_sdk/WindowManager.h>
#include <imgui.h>
#include <libassets/util/Logger.h>
#include <memory>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <utility>

WindowManager &WindowManager::Get()
{
    static WindowManager windowManagerSingleton{};
    return windowManagerSingleton;
}

bool WindowManager::Init(const std::string &appName)
{
    Logger::Info("Starting {}...", appName);

    (void)SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, appName.c_str());
    (void)SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Droc101 Development");
    (void)SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "application");
    (void)SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, "https://github.com/droc101/game-sdk");

    (void)SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");

#ifdef SDL_PLATFORM_LINUX
    (void)SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");
    (void)SDL_SetHint(SDL_HINT_VIDEO_FORCE_EGL, "1");
#endif

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        Logger::Error("SDL_Init() failed: {}", SDL_GetError());
        return false;
    }

    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0))
    {
        Logger::Error("SDL_GL_SetAttribute() failed: {}", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE))
    {
        Logger::Error("SDL_GL_SetAttribute() failed: {}", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4))
    {
        Logger::Error("SDL_GL_SetAttribute() failed: {}", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6))
    {
        Logger::Error("SDL_GL_SetAttribute() failed: {}", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1))
    {
        Logger::Error("SDL_GL_SetAttribute() failed: {}", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0))
    {
        Logger::Error("SDL_GL_SetAttribute() failed: {}", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0))
    {
        Logger::Error("SDL_GL_SetAttribute() failed: {}", SDL_GetError());
    }

    SharedMgr::Get().InitSharedMgr();

    return true;
}

SDL_GLContext WindowManager::CreateGlContext(SDL_Window *window)
{
    if (!SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, firstGlContextCreated ? 1 : 0))
    {
        Logger::Error("Failed to set OpenGL context sharing: {}", SDL_GetError());
    }
    const SDL_GLContext ctx = SDL_GL_CreateContext(window);
    if (ctx == nullptr)
    {
        Logger::Error("SDL_GL_CreateContext() failed: {}", SDL_GetError());
        return nullptr;
    }

    if (!firstGlContextCreated)
    {
        if (!GLHelper::Init())
        {
            return nullptr;
        }

        if (!ModelViewer::GlobalInit())
        {
            return nullptr;
        }

        SharedMgr::Get().textureCache.InitMissingTexture();

        firstGlContextCreated = true;
    }

    if (!SDL_GL_SetSwapInterval(1)) // Enable vsync
    {
        Logger::Error("SDL_GL_SetSwapInterval() failed: {}", SDL_GetError());
    }

    return ctx;
}

void WindowManager::Destroy()
{
    ModelViewer::GlobalDestroy();
    SharedMgr::Get().DestroySharedMgr();
    SDL_Quit();
}

int WindowManager::Loop()
{
    while (true)
    {
        for (const std::pair<std::shared_ptr<Window>, std::shared_ptr<Window>> &window: windowsToAdd)
        {
            Window *w = window.second.get();
            workingWindow = window.second;
            if (!w->BaseInit(window.first))
            {
                Logger::Error("Failed to init window!");
                exit(1);
            }
            windows.push_back(window.second);
        }
        windowsToAdd.clear();

        for (const std::shared_ptr<Window> &window: windows)
        {
            ProcessEventQueue();
            if (window != nullptr && !window->NeedsInit() && !window->HasCloseRequest())
            {
                workingWindow = window;
                window->BaseProcess();
            }
        }

        for (const std::shared_ptr<Window> &window: windows)
        {
            if (window->HasCloseRequest())
            {
                window->BaseDestroy();
            }
        }

        std::erase_if(windows, [](const std::shared_ptr<Window> &w) { return w.get()->IsDestroyed(); });

        if (windows.empty())
        {
            Destroy();
            return 0;
        }
    }
}

void WindowManager::ProcessEventQueue()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        for (const std::shared_ptr<Window> &window: windows)
        {
            Window *w = window.get();
            const SDL_Window *sdlWnd = SDL_GetWindowFromEvent(&event);
            if (sdlWnd == nullptr || sdlWnd == w->GetWindow())
            {
                workingWindow = window;
                w->BaseProcessEvent(&event);
            }
        }
    }
}

void WindowManager::ApplyTheme() const
{
    ImGuiContext *ctx = ImGui::GetCurrentContext();
    for (const std::shared_ptr<Window> &window: windows)
    {
        if (!window.get()->NeedsInit())
        {
            window.get()->ApplyTheme();
        }
    }
    ImGui::SetCurrentContext(ctx);
}

Window *WindowManager::GetCurrentWindow() const
{
    return workingWindow.get();
}
