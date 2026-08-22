//
// Created by droc101 on 2/2/26.
//

#include <game_sdk/SDKWindow.h>
#include <imgui.h>
#include <SDL3/SDL_dialog.h>
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
    return false;
}

void SDKWindow::SetWindowIcon(const std::string &iconName) const {}

void SDKWindow::MainLoop(const SDKWindowRenderFunction Render, const SDKWindowProcessEventFunction ProcessEvent) {}

SDL_Window *SDKWindow::GetWindow() const
{
    return nullptr;
}

void SDKWindow::PostQuit() {}

void SDKWindow::Destroy() const {}

void SDKWindow::ErrorMessage(const std::string &body, const std::string &title) const {}

void SDKWindow::WarningMessage(const std::string &body, const std::string &title) const {}

void SDKWindow::InfoMessage(const std::string &body, const std::string &title) const {}

void SDKWindow::FileDialogMainThreadCallback(void *userdata) {}

void SDKWindow::MultiFileDialogMainThreadCallback(void *userdata) {}

void SDKWindow::MultiFileDialogCallback(void *callbackPtr, const char *const *fileList, int /*filter*/) {}

void SDKWindow::FileDialogCallback(void *callbackPtr, const char *const *fileList, int /*filter*/) {}

void SDKWindow::OpenFileDialog(const SDKWindowFileDialogCallback Callback,
                               const std::vector<SDL_DialogFileFilter> &filters) const
{}

void SDKWindow::OpenMultiFileDialog(const SDKWindowMultiFileDialogCallback Callback,
                                    const std::vector<SDL_DialogFileFilter> &filters) const
{}

void SDKWindow::SaveFileDialog(const SDKWindowFileDialogCallback Callback,
                               const std::vector<SDL_DialogFileFilter> &filters) const
{}

void SDKWindow::OpenFolderDialog(const SDKWindowFileDialogCallback Callback) const {}

void SDKWindow::ApplyTheme() const {}

ImFont *SDKWindow::GetNormalFont() const
{
    return nullptr;
}

ImFont *SDKWindow::GetMonospaceFont() const
{
    return nullptr;
}

void SDKWindow::SetThemeChangeCallback(const SDKWindowThemeChangeCallback Callback) {}
