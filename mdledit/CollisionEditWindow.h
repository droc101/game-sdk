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
        static inline bool visible = false;

        static void RenderCHullUI(SDL_Window *window);

        static void RenderStaticMeshUI(SDL_Window *window);

        static void ImportStaticMeshCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/);

        static void AddSingleHullCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/);

        static void AddMultipleHullsCallback(void * /*userdata*/, const char *const *fileList, int /*filter*/);
};
