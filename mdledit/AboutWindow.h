//
// Created by droc101 on 6/29/25.
//

#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H
#include <SDL3/SDL_video.h>


class AboutWindow {
    public:
        static void Show();
        static void Hide();
        static void Render(SDL_Window * window);
    private:
        inline static bool visible = false;
};



#endif //ABOUTWINDOW_H
