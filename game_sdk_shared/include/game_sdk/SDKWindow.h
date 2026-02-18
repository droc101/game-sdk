//
// Created by droc101 on 2/2/26.
//

#pragma once

#include <glm/vec2.hpp>
#include <imgui.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <vector>

/**
 * Function signature for main loop render callback
 */
using SDKWindowRenderFunction = void (*)();

/**
 * Function signature for theme changes
 */
using SDKWindowThemeChangeCallback = void(*)();

/**
 * Function signature for main loop event handling callback
 * @param event The event to handle
 * @return whether the event was handled
 */
using SDKWindowProcessEventFunction = bool (*)(SDL_Event *event);

/**
 * Function signature for single file/folder dialogs
 * @param path The path the user chose
 */
using SDKWindowFileDialogCallback = void (*)(const std::string &path);
/**
 * Function signature for multi-file open dialog
 * @param paths The paths the user chose
 */
using SDKWindowMultiFileDialogCallback = void (*)(const std::vector<std::string> &paths);

class SDKWindow
{
    public:
        static SDKWindow &Get();

        /**
         * Initialize GAME SDK and create the main window
         * @param appName The name of this program, used for the window title
         * @param windowSize The default size of the main window
         * @param windowFlags SDL window flags for the main window, SDL_WINDOW_RESIZABLE by default
         * @return True on success, false on failure
         */
        [[nodiscard]] bool Init(const std::string &appName,
                                glm::ivec2 windowSize = {800, 600},
                                SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE);

        /**
         * Run the main loop
         * @param Render The render function
         * @param ProcessEvent The event handler function, nullptr by default
         */
        void MainLoop(SDKWindowRenderFunction Render, SDKWindowProcessEventFunction ProcessEvent = nullptr);

        /**
         * Get the main window's pointer
         */
        [[nodiscard]] SDL_Window *GetWindow();

        /**
         * Instruct the main loop to exit at the end of the current iteration
         */
        void PostQuit();

        /**
         * Destroy the SDKWindow resources
         */
        void Destroy();

        /**
         * Show an error message
         * @param body Message text
         * @param title Message title
         */
        void ErrorMessage(const std::string &body, const std::string &title = "Error");

        /**
         * Show a warning message
         * @param body Message text
         * @param title Message title
         */
        void WarningMessage(const std::string &body, const std::string &title = "Warning");

        /**
         * Show an info message
         * @param body Message text
         * @param title Message title
         */
        void InfoMessage(const std::string &body, const std::string &title);

        /**
         * Show an open file dialog
         * @param Callback The callback to call on success
         * @param filters File type filters
         * @note The callback will only be called on success (user chose a file), and will be called on the main thread.
         */
        void OpenFileDialog(SDKWindowFileDialogCallback Callback, const std::vector<SDL_DialogFileFilter> &filters);

        /**
         * Show an open multiple files dialog
         * @param Callback The callback to call on success
         * @param filters File type filters
         * @note The callback will only be called on success (user chose one or more files), and will be called on the main thread.
         */
        void OpenMultiFileDialog(SDKWindowMultiFileDialogCallback Callback,
                                 const std::vector<SDL_DialogFileFilter> &filters);

        /**
         * Show a save file dialog
         * @param Callback The callback to call on success
         * @param filters File type filters
         * @note The callback will only be called on success (user chose a file), and will be called on the main thread.
         */
        void SaveFileDialog(SDKWindowFileDialogCallback Callback, const std::vector<SDL_DialogFileFilter> &filters);

        /**
         * Show an open folder dialog
         * @param Callback The callback to call on success
         * @note The callback will only be called on success (user chose a folder), and will be called on the main thread.
         */
        void OpenFolderDialog(SDKWindowFileDialogCallback Callback);

        /**
         * Apply theme from options
         */
        void ApplyTheme() const;

        /**
         * Get the default ImGui font
         */
        [[nodiscard]] ImFont *GetNormalFont() const;

        /**
         * Get the monospace ImGui font
         */
        [[nodiscard]] ImFont *GetMonospaceFont() const;

        /**
         * Set the callback function that is called when the theme changes
         */
        void SetThemeChangeCallback(SDKWindowThemeChangeCallback callback);

    private:
        bool initDone = false;
        SDL_Window *window = nullptr;
        SDL_GLContext glContext = nullptr;
        bool quitRequest = false;
        ImFont *normalFont = nullptr;
        ImFont *monospaceFont = nullptr;
        SDKWindowThemeChangeCallback ThemeChangeCallback = nullptr;

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
