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
        SDKWindow() = default;

        [[nodiscard]] bool Init(const std::string &appName,
                                glm::ivec2 windowSize = {800, 600},
                                SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE);

        void MainLoop(SDKWindowRenderFunction Render, SDKWindowProcessEventFunction ProcessEvent = nullptr) const;

        [[nodiscard]] SDL_Window *GetWindow();

        void PostQuit();

        ~SDKWindow();

    private:
        bool initDone = false;
        SDL_Window *window = nullptr;
        SDL_GLContext glContext = nullptr;
        bool quitRequest = false;
};
