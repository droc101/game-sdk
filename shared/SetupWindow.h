//
// Created by droc101 on 11/12/25.
//

#pragma once
#include <SDL3/SDL_video.h>


class SetupWindow
{
    public:
        SetupWindow() = delete;
        static void Show(bool required = true);
        static void Render(SDL_Window *window);

    private:
        static inline bool visible = false;
        static inline bool required = true;

        static void gamePathCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/);

        static void assetsPathCallback(void * /*userdata*/, const char *const *filelist, int /*filter*/);
};
