//
// Created by droc101 on 2/2/26.
//

#pragma once

#include <glm/vec2.hpp>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <string>

using SDKWindowRenderFunction = void (*)(SDL_Window *window);
using SDKWindowProcessEventFunction = bool (*)(SDL_Event *window);

class SDKWindow
{
    public:
        SDKWindow() = delete;

        [[nodiscard]] static bool Init(const std::string &appName,
                                glm::ivec2 windowSize = {800, 600},
                                SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE);

        static void MainLoop(SDKWindowRenderFunction Render, SDKWindowProcessEventFunction ProcessEvent = nullptr);

        [[nodiscard]] static SDL_Window *GetWindow();

        static void PostQuit();

        static void Destroy();

        static void ErrorMessage(const std::string &body, const std::string &title = "Error");

        static void WarningMessage(const std::string &body, const std::string &title = "Warning");

        static void InfoMessage(const std::string &body, const std::string &title);

    private:
        static inline bool initDone = false;
        static inline SDL_Window *window = nullptr;
        static inline SDL_GLContext glContext = nullptr;
        static inline bool quitRequest = false;
};
