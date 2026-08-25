//
// Created by droc101 on 8/20/26.
//

#pragma once

#include <game_sdk/Options.h>
#include <game_sdk/Window.h>
#include <libassets/util/ArgumentParser.h>
#include <memory>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

template<typename T> concept DerivedWindow = std::is_base_of_v<Window, T>;

class WindowManager
{
    public:

        /**
         * Run and SDK app using the WindowManager system
         * @tparam T The main window class
         * @param argc main function argc
         * @param argv main function argv
         * @param appName The name of the app
         * @param requireGamePath Whether or not to require a valid game path to launch this program
         * @return Process return code
         */
        template<DerivedWindow T>
        static int Run(const int argc, const char **argv, const std::string &appName, const bool requireGamePath = true)
        {
            WindowManager &mgr = Get();
            if (!mgr.Init(appName, argc, argv))
            {
                return 1;
            }

            if (requireGamePath && !Options::Get().ValidateGamePath())
            {
                (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                               "Configuration Error",
                                               "An error was detected in the game path configuration. Please correct "
                                               "the configuration from the GAME SDK launcher.",
                                               nullptr);
                return 2;
            }

            mgr.AddWindow<T>();
            return mgr.Loop();
        }

        /**
         * Get the @c WindowManager singleton
         */
        static WindowManager &Get();

        /**
         * Get or create an OpenGL context
         * @param window The window to create an OpenGL context for
         * @note This will do additional initialization for the first context created
         */
        SDL_GLContext CreateGlContext(SDL_Window *window);

        /**
         * Add a non-modal window
         */
        template<DerivedWindow T, typename... Args> void AddWindow(Args &&...args)
        {
            windowsToAdd.emplace_back(nullptr, std::make_shared<T>(std::forward<Args>(args)...));
        }

        /**
         * Add a window as a modal to the currently processing window
         */
        template<DerivedWindow T, typename... Args> void AddModalWindow(Args &&...args)
        {
            workingWindow->ModalBlock();
            windowsToAdd.emplace_back(workingWindow, std::make_shared<T>(std::forward<Args>(args)...));
        }

        /**
         * Apply the ImGui theme from SDK options
         */
        void ApplyTheme() const;

        /**
         * Get the currently processing window
         */
        [[nodiscard]] Window *GetCurrentWindow() const;

        const ArgumentParser &GetArgumentParser() const;

    private:
        /// Whether the first OpenGL context has been created
        bool firstGlContextCreated = false;
        /// The currently processing window
        std::shared_ptr<Window> workingWindow = nullptr;
        /// Windows to be added next iteration, .first is the parent window, .second is the new window to add
        std::vector<std::pair<std::shared_ptr<Window>, std::shared_ptr<Window>>> windowsToAdd{};
        /// Windows to process
        std::vector<std::shared_ptr<Window>> windows{};
        ArgumentParser args{};

        /**
         * Initialize the window manager
         * @return Success or failure
         */
        bool Init(const std::string &appName, int argc, const char **argv);

        /**
         * Run the window manager
         * @return Process return code
         */
        int Loop();

        /**
         * Process the SDL event queue
         */
        void ProcessEventQueue();

        /**
         * Destroy the window manager
         */
        void Destroy();
};
