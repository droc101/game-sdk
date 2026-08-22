//
// Created by droc101 on 2/2/26.
//

// Legacy SDK window stubs

#pragma once

#include <glm/vec2.hpp>
#include <imgui.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <vector>

using SDKWindowRenderFunction = void (*)();
using SDKWindowThemeChangeCallback = void (*)();
using SDKWindowProcessEventFunction = bool (*)(SDL_Event *event);
using SDKWindowFileDialogCallback = void (*)(const std::string &path);
using SDKWindowMultiFileDialogCallback = void (*)(const std::vector<std::string> &paths);

class SDKWindow
{
    public:
        static SDKWindow &Get();
        [[nodiscard]] bool Init(const std::string &appName,
                                glm::ivec2 windowSize = {800, 600},
                                SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE);
        void SetWindowIcon(const std::string &iconName) const;
        void MainLoop(SDKWindowRenderFunction Render, SDKWindowProcessEventFunction ProcessEvent = nullptr);
        [[nodiscard]] SDL_Window *GetWindow() const;
        void PostQuit();
        void Destroy() const;
        void ErrorMessage(const std::string &body, const std::string &title = "Error") const;
        void WarningMessage(const std::string &body, const std::string &title = "Warning") const;
        void InfoMessage(const std::string &body, const std::string &title) const;
        void OpenFileDialog(SDKWindowFileDialogCallback Callback,
                            const std::vector<SDL_DialogFileFilter> &filters) const;
        void OpenMultiFileDialog(SDKWindowMultiFileDialogCallback Callback,
                                 const std::vector<SDL_DialogFileFilter> &filters) const;
        void SaveFileDialog(SDKWindowFileDialogCallback Callback,
                            const std::vector<SDL_DialogFileFilter> &filters) const;
        void OpenFolderDialog(SDKWindowFileDialogCallback Callback) const;
        void ApplyTheme() const;
        [[nodiscard]] ImFont *GetNormalFont() const;
        [[nodiscard]] ImFont *GetMonospaceFont() const;
        void SetThemeChangeCallback(SDKWindowThemeChangeCallback Callback);

    private:
        struct FileDialogMainThreadCallbackData
        {
                SDKWindowFileDialogCallback Callback;
                std::string path;
        };
        struct MultiFileDialogMainThreadCallbackData
        {
                SDKWindowMultiFileDialogCallback Callback;
                std::vector<std::string> paths;
        };
        SDKWindow() = default;
        static void FileDialogMainThreadCallback(void *userdata);
        static void FileDialogCallback(void *callbackPtr, const char *const *fileList, int filter);
        static void MultiFileDialogMainThreadCallback(void *userdata);
        static void MultiFileDialogCallback(void *callbackPtr, const char *const *fileList, int filter);
};
