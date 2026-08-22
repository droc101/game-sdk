//
// Created by droc101 on 8/20/26.
//

#include <cassert>
#include <cstdint>
#include <game_sdk/Options.h>
#include <game_sdk/SharedMgr.h>
#include <game_sdk/Window.h>
#include <game_sdk/WindowManager.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <libassets/asset/TextureAsset.h>
#include <libassets/util/Error.h>
#include <libassets/util/Logger.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <utility>
#include <vector>

bool Window::BaseInit()
{
    SharedMgr::Get().InitSharedMgr();

    const WindowProperties &props = this->GetProperties();

    const SDL_WindowFlags sdlWindowFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | props.defaultFlags;
    window = SDL_CreateWindow(props.title.c_str(), props.defaultSize.x, props.defaultSize.y, sdlWindowFlags);
    if (window == nullptr)
    {
        Logger::Error("SDL_CreateWindow() failed: {}", SDL_GetError());
        return false;
    }

    if (!props.icon.empty())
    {
        SetWindowIcon(props.icon);
    }

    glContext = WindowManager::Get().GetOrCreateContext(window);

    if (!SDL_GL_MakeCurrent(window, glContext))
    {
        Logger::Error("SDL_GL_MakeCurrent() failed: {}", SDL_GetError());
        return false;
    }

    IMGUI_CHECKVERSION();
    icx = ImGui::CreateContext();
    ImGui::SetCurrentContext(icx);
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ApplyTheme();

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);

    normalFont = io.Fonts->AddFontFromFileTTF("assets/fonts/NotoSans.ttf");
    monospaceFont = io.Fonts->AddFontFromFileTTF("assets/fonts/JetBrainsMono.ttf");

    const char *glslVersion = "#version 460";

    ImGui_ImplOpenGL3_Init(glslVersion);

    if (!SDL_ShowWindow(window))
    {
        Logger::Error("SDL_ShowWindow() failed: {}", SDL_GetError());
        return false;
    }

    if (!this->Init())
    {
        return false;
    }

    initDone = true;
    return true;
}

void Window::BaseProcessEvent(SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        closeRequest = true;
    }
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        closeRequest = true;
    } else if (!ProcessEvent(event))
    {
        MakeCurrent();
        ImGui_ImplSDL3_ProcessEvent(event);
    }
}

void Window::BaseProcess()
{
    MakeCurrent();

    if ((SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0)
    {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    const WindowProperties &props = GetProperties();

    if (props.defaultImguiWindow)
    {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        constexpr ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoDecoration |
                                                  ImGuiWindowFlags_NoMove |
                                                  ImGuiWindowFlags_NoSavedSettings |
                                                  ImGuiWindowFlags_NoBringToFrontOnFocus;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin(GetProperties().title.c_str(), nullptr, WINDOW_FLAGS);
        ImGui::PopStyleVar();
    }

    this->Render();

    if (props.defaultImguiWindow)
    {
        ImGui::End();
    }

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (!SDL_GL_SwapWindow(window))
    {
        Logger::Error("SDL_GL_SwapWindow() failed: {}", SDL_GetError());
    }

    if (closeRequest)
    {
        BaseDestroy();
    }
}

void Window::BaseDestroy()
{
    this->Destroy();
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext(icx);
    if (!SDL_GL_DestroyContext(glContext))
    {
        Logger::Error("SDL_GL_DestroyContext() failed: {}", SDL_GetError());
    }
    SDL_DestroyWindow(window);
    window = nullptr;
}

bool Window::IsDestroyed() const
{
    return initDone && window == nullptr;
}

bool Window::NeedsInit() const
{
    return !initDone;
}

SDL_Window *Window::GetWindow() const
{
    return window;
}

void Window::ApplyTheme() const
{
    ImGui::SetCurrentContext(icx);
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
    this->ThemeChanged();
}

void Window::MakeCurrent()
{
    if (!SDL_GL_MakeCurrent(window, glContext))
    {
        Logger::Error("SDL_GL_MakeCurrent() failed: {}", SDL_GetError());
        return;
    }

    ImGui::SetCurrentContext(icx);
}

void Window::SetWindowIcon(const std::string &iconName) const
{
    TextureAsset iconAsset{};
    const Error::ErrorCode e = iconAsset.Import("assets/icons/" + iconName + ".png");
    if (e != Error::ErrorCode::OK)
    {
        iconAsset.CreateMissingTexture();
    }
    uint8_t *pixels = iconAsset.GetPixelsRGBA();
    assert(iconAsset.GetFormat() == TextureAsset::PixelFormat::RGBA8);
    SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(iconAsset.GetWidth()),
                                                 static_cast<int>(iconAsset.GetHeight()),
                                                 SDL_PIXELFORMAT_ABGR8888,
                                                 pixels,
                                                 static_cast<int>(sizeof(uint32_t) * iconAsset.GetWidth()));
    if (surface == nullptr)
    {
        Logger::Error("SDL_CreateSurfaceFrom failed: {}", SDL_GetError());
    } else
    {
        if (!SDL_SetWindowIcon(window, surface))
        {
            Logger::Error("SDL_SetWindowIcon failed: {}", SDL_GetError());
        }
    }
    SDL_DestroySurface(surface);
}

#pragma region Default Virtual Functions

bool Window::ProcessEvent(SDL_Event *event)
{
    (void)event;
    return false;
}

void Window::Render() {}

void Window::ThemeChanged() const {}

const Window::WindowProperties &Window::GetProperties() const
{
    return defaultProperties;
}

bool Window::Init()
{
    return true;
}

void Window::Destroy() {}

#pragma endregion

#pragma region Message Boxes
void Window::ErrorMessage(const std::string &body, const std::string &title) const
{
    (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), body.c_str(), window);
}

void Window::WarningMessage(const std::string &body, const std::string &title) const
{
    (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, title.c_str(), body.c_str(), window);
}

void Window::InfoMessage(const std::string &body, const std::string &title) const
{
    (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title.c_str(), body.c_str(), window);
}
#pragma endregion

#pragma region File Dialogs
void Window::SDLFileDialogMainThreadCallback(void *userdata)
{
    const FileDialogCallbackData *data = static_cast<FileDialogCallbackData *>(userdata);
    data->Callback(data->path);
    delete data;
}

void Window::SDLMultiFileDialogMainThreadCallback(void *userdata)
{
    const MultiFileDialogCallbackData *data = static_cast<MultiFileDialogCallbackData *>(userdata);
    data->Callback(data->paths);
    delete data;
}

void Window::SDLMultiFileDialogCallback(void *callbackPtr, const char *const *fileList, int /*filter*/)
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

    MultiFileDialogCallbackData *data = static_cast<MultiFileDialogCallbackData *>(callbackPtr);
    data->paths = std::move(files);

    if (!SDL_RunOnMainThread(SDLMultiFileDialogMainThreadCallback, data, true))
    {
        Logger::Error("Failed to call MultiFileDialogMainThreadCallback on main thread: {}", SDL_GetError());
    }
}

void Window::SDLFileDialogCallback(void *callbackPtr, const char *const *fileList, int /*filter*/)
{
    if (fileList == nullptr || fileList[0] == nullptr)
    {
        return;
    }

    FileDialogCallbackData *data = static_cast<FileDialogCallbackData *>(callbackPtr);
    data->path = fileList[0];

    if (!SDL_RunOnMainThread(SDLFileDialogMainThreadCallback, data, true))
    {
        Logger::Error("Failed to call FileDialogMainThreadCallback on main thread: {}", SDL_GetError());
    }
}

void Window::OpenFileDialog(FileDialogCallback &&Callback, const std::vector<SDL_DialogFileFilter> &filters) const
{
    FileDialogCallbackData *data = new FileDialogCallbackData();
    data->Callback = std::move(Callback);

    SDL_ShowOpenFileDialog(SDLFileDialogCallback,
                           data,
                           window,
                           filters.data(),
                           static_cast<int>(filters.size()),
                           nullptr,
                           false);
}

void Window::OpenMultiFileDialog(MultiFileDialogCallback &&Callback,
                                 const std::vector<SDL_DialogFileFilter> &filters) const
{
    MultiFileDialogCallbackData *data = new MultiFileDialogCallbackData();
    data->Callback = std::move(Callback);

    SDL_ShowOpenFileDialog(SDLMultiFileDialogCallback,
                           data,
                           window,
                           filters.data(),
                           static_cast<int>(filters.size()),
                           nullptr,
                           true);
}

void Window::SaveFileDialog(FileDialogCallback &&Callback, const std::vector<SDL_DialogFileFilter> &filters) const
{
    FileDialogCallbackData *data = new FileDialogCallbackData();
    data->Callback = std::move(Callback);

    SDL_ShowSaveFileDialog(SDLFileDialogCallback,
                           data,
                           window,
                           filters.data(),
                           static_cast<int>(filters.size()),
                           nullptr);
}

void Window::OpenFolderDialog(FileDialogCallback &&Callback) const
{
    FileDialogCallbackData *data = new FileDialogCallbackData();
    data->Callback = std::move(Callback);

    SDL_ShowOpenFolderDialog(SDLFileDialogCallback, data, window, nullptr, false);
}
#pragma endregion
