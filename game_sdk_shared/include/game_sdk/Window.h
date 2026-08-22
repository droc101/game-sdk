//
// Created by droc101 on 8/20/26.
//

#pragma once

#include <functional>
#include <glm/vec2.hpp>
#include <imgui.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <vector>

#define FILE_DIALOG_CALLBACK(Method) \
    [this](const std::string &path) { \
        (Method)(path); \
    }
#define MULTI_FILE_DIALOG_CALLBACK(Method) \
    [this](const std::vector<std::string> &path) { \
        (Method)(path); \
    }

class Window
{
    public:
        Window() = default;
        virtual ~Window() = default;

        [[nodiscard]] bool BaseInit();

        void BaseProcessEvent(SDL_Event *event);

        void BaseProcess();

        void ApplyTheme() const;

        [[nodiscard]] bool IsDestroyed() const;
        [[nodiscard]] bool NeedsInit() const;

        [[nodiscard]] SDL_Window *GetWindow() const;

    protected:

        /**
         * Function signature for single file/folder dialogs
         * @param path The path the user chose
         */
        using FileDialogCallback = std::function<void(const std::string &)>;
        /**
         * Function signature for multi-file open dialog
         * @param paths The paths the user chose
         */
        using MultiFileDialogCallback = std::function<void(const std::vector<std::string> &paths)>;


        SDL_Window *window = nullptr;
        bool closeRequest = false;
        ImFont *normalFont = nullptr;
        ImFont *monospaceFont = nullptr;

        struct WindowProperties
        {
                std::string title;
                glm::ivec2 defaultSize;
                std::string icon;
                SDL_WindowFlags defaultFlags;
                bool defaultImguiWindow;
        };

        void ErrorMessage(const std::string &body, const std::string &title = "Error") const;

        void WarningMessage(const std::string &body, const std::string &title = "Warning") const;

        void InfoMessage(const std::string &body, const std::string &title) const;

        void OpenFileDialog(FileDialogCallback &&Callback, const std::vector<SDL_DialogFileFilter> &filters) const;

        void OpenMultiFileDialog(MultiFileDialogCallback &&Callback,
                                 const std::vector<SDL_DialogFileFilter> &filters) const;

        void SaveFileDialog(FileDialogCallback &&Callback, const std::vector<SDL_DialogFileFilter> &filters) const;

        void OpenFolderDialog(FileDialogCallback &&Callback) const;

        [[nodiscard]] virtual const WindowProperties &GetProperties() const;

        virtual bool Init();

        virtual void Destroy();

        virtual void Render();

        virtual bool ProcessEvent(SDL_Event *event);

        virtual void ThemeChanged() const;

    private:
        SDL_GLContext glContext = nullptr;
        ImGuiContext *icx = nullptr;
        bool initDone = false;

        struct FileDialogCallbackData
        {
                FileDialogCallback Callback;
                std::string path = "";
        };

        struct MultiFileDialogCallbackData
        {
                MultiFileDialogCallback Callback = nullptr;
                std::vector<std::string> paths{};
        };

        void MakeCurrent();

        void SetWindowIcon(const std::string &iconName) const;

        void BaseDestroy();

        static void SDLFileDialogMainThreadCallback(void *userdata);

        static void SDLFileDialogCallback(void *callbackPtr, const char *const *fileList, int filter);

        static void SDLMultiFileDialogMainThreadCallback(void *userdata);

        static void SDLMultiFileDialogCallback(void *callbackPtr, const char *const *fileList, int filter);

        WindowProperties defaultProperties = {
            .title = "GAME SDK",
            .defaultSize = glm::ivec2(800, 600),
            .icon = "sdk_hires",
            .defaultFlags = 0,
            .defaultImguiWindow = true,
        };
};
