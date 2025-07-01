//
// Created by droc101 on 6/29/25.
//

#pragma once

#include <SDL3/SDL_video.h>

class OptionsWindow
{
    public:
        OptionsWindow() = delete;

        static void Show();
        static void Hide();
        static void Render(SDL_Window *window);

    private:
        static inline bool visible = false;

        static void gamePathCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/);
};
