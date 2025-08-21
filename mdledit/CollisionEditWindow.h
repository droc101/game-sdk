//
// Created by droc101 on 8/20/25.
//

#pragma once

#include <SDL3/SDL_video.h>

class CollisionEditWindow
{
    public:
        CollisionEditWindow() = delete;

        static void Show();

        static void Hide();

        static void Render(SDL_Window *window);

    private:
        inline static bool visible = false;
};
