//
// Created by droc101 on 2/2/26.
//

#include <cassert>
#include <cstdio>
#include <game_sdk/gl/GLHelper.h>
#include <game_sdk/Options.h>
#include <game_sdk/SDKWindow.h>
#include <game_sdk/SharedMgr.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <vector>

SDKWindow &SDKWindow::Get()
{
    static SDKWindow sdkWindowSingleton{};

    return sdkWindowSingleton;
}

bool SDKWindow::Init(const std::string &appName, const glm::ivec2 windowSize, const SDL_WindowFlags windowFlags)
{
    assert(!initDone);

    printf("Starting %s...\n", appName.c_str());

#ifdef SDL_PLATFORM_LINUX
    (void)SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");
    (void)SDL_SetHint(SDL_HINT_VIDEO_FORCE_EGL, "1");
#endif

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return false;
    }

    SharedMgr::Get().InitSharedMgr();

    const char *glslVersion = "#version 330";
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

    SharedMgr::Get().textureCache.InitMissingTexture();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    normalFont = io.Fonts->AddFontFromFileTTF("assets/fonts/NotoSans.ttf");
    monospaceFont = io.Fonts->AddFontFromFileTTF("assets/fonts/JetBrainsMono.ttf");

    ApplyTheme();

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

void SDKWindow::MainLoop(const SDKWindowRenderFunction Render, const SDKWindowProcessEventFunction ProcessEvent)
{
    assert(initDone);
    while (true)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                quitRequest = true;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
            {
                quitRequest = true;
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

        Render();

        SharedMgr::Get().RenderSharedUI();
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (!SDL_GL_SwapWindow(window))
        {
            printf("Error: SDL_GL_SwapWindow(): %s\n", SDL_GetError());
        }

        if (quitRequest)
        {
            break;
        }
    }
}

SDL_Window *SDKWindow::GetWindow()
{
    assert(initDone);
    return window;
}

void SDKWindow::PostQuit()
{
    assert(initDone);
    quitRequest = true;
}

void SDKWindow::Destroy()
{
    assert(initDone);
    SharedMgr::Get().DestroySharedMgr();
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

void SDKWindow::ErrorMessage(const std::string &body, const std::string &title)
{
    (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), body.c_str(), window);
}

void SDKWindow::WarningMessage(const std::string &body, const std::string &title)
{
    (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, title.c_str(), body.c_str(), window);
}

void SDKWindow::InfoMessage(const std::string &body, const std::string &title)
{
    (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title.c_str(), body.c_str(), window);
}

void SDKWindow::FileDialogMainThreadCallback(void *userdata)
{
    const FileDialogMainThreadCallbackData *data = static_cast<FileDialogMainThreadCallbackData *>(userdata);
    data->Callback(data->path);
}

void SDKWindow::MultiFileDialogMainThreadCallback(void *userdata)
{
    const MultiFileDialogMainThreadCallbackData *data = static_cast<MultiFileDialogMainThreadCallbackData *>(userdata);
    data->Callback(data->paths);
}

void SDKWindow::MultiFileDialogCallback(void *userdata, const char *const *fileList, int filter)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }

    std::vector<std::string> files{};
    while (*fileList != nullptr)
    {
        files.emplace_back(*fileList);
        fileList++;
    }

    const SDKWindowMultiFileDialogCallback Callback = reinterpret_cast<SDKWindowMultiFileDialogCallback>(userdata);
    if (SDL_IsMainThread())
    {
        Callback(files);
    } else
    {
        MultiFileDialogMainThreadCallbackData data = {.Callback = Callback, .paths = files};
        if (!SDL_RunOnMainThread(MultiFileDialogMainThreadCallback, &data, true))
        {
            printf("Failed to call MultiFileDialogMainThreadCallback on main thread: %s\n", SDL_GetError());
        }
    }
}

void SDKWindow::FileDialogCallback(void *userdata, const char *const *fileList, int filter)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }

    const SDKWindowFileDialogCallback Callback = reinterpret_cast<SDKWindowFileDialogCallback>(userdata);
    if (SDL_IsMainThread())
    {
        Callback(fileList[0]);
    } else
    {
        FileDialogMainThreadCallbackData data = {.Callback = Callback, .path = fileList[0]};
        if (!SDL_RunOnMainThread(FileDialogMainThreadCallback, &data, true))
        {
            printf("Failed to call FileDialogMainThreadCallback on main thread: %s\n", SDL_GetError());
        }
    }
}

void SDKWindow::OpenFileDialog(const SDKWindowFileDialogCallback Callback,
                               const std::vector<SDL_DialogFileFilter> &filters)
{
    SDL_ShowOpenFileDialog(FileDialogCallback,
                           reinterpret_cast<void *>(Callback),
                           GetWindow(),
                           filters.data(),
                           static_cast<int>(filters.size()),
                           nullptr,
                           false);
}

void SDKWindow::OpenMultiFileDialog(SDKWindowMultiFileDialogCallback Callback,
                                    const std::vector<SDL_DialogFileFilter> &filters)
{
    SDL_ShowOpenFileDialog(MultiFileDialogCallback,
                           reinterpret_cast<void *>(Callback),
                           GetWindow(),
                           filters.data(),
                           static_cast<int>(filters.size()),
                           nullptr,
                           true);
}

void SDKWindow::SaveFileDialog(const SDKWindowFileDialogCallback Callback,
                               const std::vector<SDL_DialogFileFilter> &filters)
{
    SDL_ShowSaveFileDialog(FileDialogCallback,
                           reinterpret_cast<void *>(Callback),
                           GetWindow(),
                           filters.data(),
                           static_cast<int>(filters.size()),
                           nullptr);
}

void SDKWindow::OpenFolderDialog(const SDKWindowFileDialogCallback Callback)
{
    SDL_ShowOpenFolderDialog(FileDialogCallback, reinterpret_cast<void *>(Callback), GetWindow(), nullptr, false);
}

void SDKWindow::ApplyTheme() const
{
    if (Options::Get().theme == Options::Theme::SYSTEM)
    {
        if (SDL_GetSystemTheme() == SDL_SYSTEM_THEME_DARK)
        {
            ImGui::StyleColorsDark();
        } else
        {
            ImGui::StyleColorsLight();
        }
    } else if (Options::Get().theme == Options::Theme::LIGHT)
    {
        ImGui::StyleColorsLight();
    } else
    {
        ImGui::StyleColorsDark();
    }
    ImGuiStyle &style = ImGui::GetStyle();
    style.FontSizeBase = 16.0;
    if (ThemeChangeCallback != nullptr)
    {
        ThemeChangeCallback();
    }
}

ImFont *SDKWindow::GetNormalFont() const
{
    return normalFont;
}

ImFont *SDKWindow::GetMonospaceFont() const
{
    return monospaceFont;
}

void SDKWindow::SetThemeChangeCallback(SDKWindowThemeChangeCallback callback)
{
    ThemeChangeCallback = callback;
}
