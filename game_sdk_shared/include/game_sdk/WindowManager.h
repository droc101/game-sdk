//
// Created by droc101 on 8/20/26.
//

#pragma once

#include <game_sdk/Window.h>
#include <memory>
#include <SDL3/SDL_video.h>
#include <string>
#include <vector>

class WindowManager
{
    public:
        static WindowManager &Get();

        bool Init(const std::string &);

        int Run();

        SDL_GLContext GetOrCreateContext(SDL_Window *window);

        void AddWindow(const std::shared_ptr<Window> &window);

        void ApplyTheme();

    private:
        SDL_GLContext glContext = nullptr;
        std::vector<std::shared_ptr<Window>> windows;

        void Destroy();
};
