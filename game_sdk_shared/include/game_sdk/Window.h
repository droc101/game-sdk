//
// Created by droc101 on 8/20/26.
//

#pragma once

#include <cstdint>
#include <functional>
#include <glm/vec2.hpp>
#include <imgui.h>
#include <memory>
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

        Window() = default;
        virtual ~Window() = default;

        /**
         * Initialize this window
         * @param modalParent If non-null, this window will be a modal blocking @c modalParent
         * @return success or failure
         */
        [[nodiscard]] bool BaseInit(const std::shared_ptr<Window> &modalParent = nullptr);

        /**
         * Process an SDL_Event
         * @param event the event to process
         */
        void BaseProcessEvent(SDL_Event *event);

        /**
         * Process and render this window
         */
        void BaseProcess();

        /**
         * Destroy this window
         */
        void BaseDestroy();

        /**
         * Apply the ImGui theme from SDK options
         */
        void ApplyTheme() const;

        /**
         * Check if this window has been destroyed
         */
        [[nodiscard]] bool IsDestroyed() const;
        /**
         * Check if this window has yet to be initalized
         */
        [[nodiscard]] bool NeedsInit() const;
        /**
         * Check if this window has a pending close request
         * @note this does NOT count close requests queued while a modal was open
         */
        [[nodiscard]] bool HasCloseRequest() const;
        /**
         * Get the normal font
         * @warning Only use this when the window's context is current
         */
        [[nodiscard]] ImFont *GetNormalFont() const;
        /**
         * Get the monospace font
         * @warning Only use this when the window's context is current
         */
        [[nodiscard]] ImFont *GetMonospaceFont() const;

        [[nodiscard]] SDL_Window *GetWindow() const;

        /**
         * Show an error message window
         * @param body Body text
         * @param title Title text
         */
        void ErrorMessage(const std::string &body, const std::string &title = "Error") const;

        /**
         * Show a warning message window
         * @param body Body text
         * @param title Title text
         */
        void WarningMessage(const std::string &body, const std::string &title = "Warning") const;

        /**
         * Show an info message window
         * @param body Body text
         * @param title Title text
         */
        void InfoMessage(const std::string &body, const std::string &title) const;

        /**
         * Show an open file dialog
         * @param Callback The function to call when the dialog is confirmed. This function will not be called if the dialog is cancelled.
         * @param filters The file types to allow selecting
         */
        void OpenFileDialog(FileDialogCallback &&Callback, const std::vector<SDL_DialogFileFilter> &filters);

        /**
         * Show an open multiple files dialog
         * @param Callback The function to call when the dialog is confirmed. This function will not be called if the dialog is cancelled.
         * @param filters The file types to allow selecting
         */
        void OpenMultiFileDialog(MultiFileDialogCallback &&Callback,
                                 const std::vector<SDL_DialogFileFilter> &filters);

        /**
         * Show an save file dialog
         * @param Callback The function to call when the dialog is confirmed. This function will not be called if the dialog is cancelled.
         * @param filters The file types to allow selecting
         */
        void SaveFileDialog(FileDialogCallback &&Callback, const std::vector<SDL_DialogFileFilter> &filters);

        /**
         * Show an open folder dialog
         * @param Callback The function to call when the dialog is confirmed. This function will not be called if the dialog is cancelled.
         */
        void OpenFolderDialog(FileDialogCallback &&Callback);

        /**
         * Indicate that this window has been blocked by a modal
         */
        void ModalBlock();
        /**
         * Indicate that this window is no longer blocked by a modal
         */
        void ModalUnblock();

        /**
         * Request that this window be closed when possible
         */
        void RequestClose();

    protected:
        struct WindowProperties
        {
                /// The title text of this window
                std::string title;
                /// The default size of this window
                glm::ivec2 defaultSize;
                /// The icon of this window, or an empty string for the OS's default
                std::string icon;
                /// The default SDL window flags
                SDL_WindowFlags defaultFlags;
                /// Whether to render an ImGui window by default
                bool defaultImguiWindow;
        };

        /**
         * Get the default properties of this window
         */
        [[nodiscard]] virtual const WindowProperties &GetProperties() const;

        /**
         * Perform subclass-specific initialization
         * @return success or failure
         */
        virtual bool Init();

        /**
         * Perform subclass-specific destruction
         */
        virtual void Destroy();

        /**
         * Perform subclass-specific processing & rendering
         */
        virtual void Render();

        /**
         * Perform subclass-specific event handling
         * @param event The event to handle
         * @return Whether the event was handled
         */
        virtual bool ProcessEvent(SDL_Event *event);

        /**
         * Perform subclass-specific theme change handling
         */
        virtual void ThemeChanged() const;

    private:
        /// The backing SDL window
        SDL_Window *window = nullptr;
        /// The backing OpenGL context
        SDL_GLContext glContext = nullptr;
        /// The backing ImGui context
        ImGuiContext *icx = nullptr;
        /// Whether this window has been initialized
        bool initDone = false;
        /// The base ImGui config flags, used when restoring changes made by modal input blocking
        ImGuiConfigFlags baseConfigFlags = 0;
        /// The number of modals directly blocking this window
        uint8_t modalCounter = 0;
        /// The parent of this window
        std::shared_ptr<Window> parent = nullptr;
        /// Whether this window should be closed whenever there is no longer a modal blocking it
        bool queueCloseWhenNotModalBlocked = false;
        /// Whether this window should be closed
        bool closeRequest = false;
        /// Normal ImGui font
        ImFont *normalFont = nullptr;
        /// Monospace ImGui font
        ImFont *monospaceFont = nullptr;

        struct FileDialogCallbackData
        {
                FileDialogCallback Callback;
                std::string path;
                Window *wnd;
        };

        struct MultiFileDialogCallbackData
        {
                MultiFileDialogCallback Callback = nullptr;
                std::vector<std::string> paths{};
                Window *wnd;
        };

        /**
         * Make this window's OpenGL and ImGui contexts current
         */
        void MakeCurrent() const;

        /**
         * Set this window's icon
         */
        void SetWindowIcon(const std::string &iconName) const;

        /**
         * File dialog callback run on the main thread
         * @param userdata Pointer to an @c FileDialogCallbackData struct that will be used and deleted
         */
        static void SDLFileDialogMainThreadCallback(void *userdata);

        /**
         * File dialog callback
         * @param callbackPtr Pointer to an @c FileDialogCallbackData struct that will be used and deleted
         * @param fileList List of selected files
         * @param filter Filter index used, discarded
         */
        static void SDLFileDialogCallback(void *callbackPtr, const char *const *fileList, int filter);

        /**
         * Multi-file dialog callback run on the main thread
         * @param userdata Pointer to an @c MultiFileDialogCallbackData struct that will be used and deleted
         */
        static void SDLMultiFileDialogMainThreadCallback(void *userdata);

        /**
         * Multi-file dialog callback
         * @param callbackPtr Pointer to an @c MultiFileDialogCallbackData struct that will be used and deleted
         * @param fileList List of selected files
         * @param filter Filter index used, discarded
         */
        static void SDLMultiFileDialogCallback(void *callbackPtr, const char *const *fileList, int filter);

        WindowProperties defaultProperties = {
            .title = "GAME SDK",
            .defaultSize = glm::ivec2(800, 600),
            .icon = "sdk_hires",
            .defaultFlags = 0,
            .defaultImguiWindow = true,
        };
};
