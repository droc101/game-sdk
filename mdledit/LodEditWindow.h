//
// Created by droc101 on 7/4/25.
//

#pragma once

#include <SDL3/SDL_video.h>

class LodEditWindow
{
    public:
        LodEditWindow() = delete;

        static void Render(SDL_Window *window);

    private:
        static void addLodCallback(void *userdata, const char *const *fileList, int filter);

        static void saveLodCallback(void *userdata, const char *const *fileList, int filter);
};
