//
// Created by droc101 on 8/20/26.
//

#pragma once

#include <game_sdk/Window.h>
#include <memory>
#include <SDL3/SDL_video.h>
#include <string>
#include <utility>
#include <vector>

class WindowManager
{
    public:
        /**
         * Get the @c WindowManager singleton
         */
        static WindowManager &Get();

        /**
         * Initialize the window manager
         * @return Success or failure
         */
        bool Init(const std::string &);

        /**
         * Run the window manager
         * @return Process return code
         */
        int Run();

        /**
         * Get or create an OpenGL context
         * @param window The window to create an OpenGL context for
         * @note This will do additional initialization for the first context created
         */
        SDL_GLContext CreateGlContext(SDL_Window *window);

        /**
         * Add a non-modal window
         * @param window The window to add
         */
        void AddWindow(const std::shared_ptr<Window> &window);

        /**
         * Add a window as a modal to the currently processing window
         * @param window The window to add
         */
        void AddModalWindow(const std::shared_ptr<Window> &window);

        /**
         * Apply the ImGui theme from SDK options
         */
        void ApplyTheme() const;

        /**
         * Get the currently processing window
         */
        [[nodiscard]] Window *GetCurrentWindow() const;

    private:
        /// Whether the first OpenGL context has been created
        bool firstGlContextCreated = false;
        /// The currently processing window
        std::shared_ptr<Window> workingWindow = nullptr;
        /// Windows to be added next iteration, .first is the parent window, .second is the new window to add
        std::vector<std::pair<std::shared_ptr<Window>, std::shared_ptr<Window>>> windowsToAdd{};
        /// Windows to process
        std::vector<std::shared_ptr<Window>> windows{};

        /**
         * Process the SDL event queue
         */
        void ProcessEventQueue();

        /**
         * Destroy the window manager
         */
        void Destroy();
};
