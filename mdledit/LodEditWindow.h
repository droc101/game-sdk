//
// Created by droc101 on 7/4/25.
//

#ifndef LODEDITWINDOW_H
#define LODEDITWINDOW_H
#include <SDL3/SDL_video.h>


class LodEditWindow {
    public:
        LodEditWindow() = delete;

        static void Show();

        static void Hide();

        static void Render(SDL_Window * window);
    private:
        static inline bool visible;

        static void addLodCallback(void * userdata, const char *const *fileList, int filter);

        static void saveLodCallback(void * userdata, const char *const *fileList, int filter);
};



#endif //LODEDITWINDOW_H
