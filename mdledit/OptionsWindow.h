//
// Created by droc101 on 6/29/25.
//

#ifndef OPTIONSWINDOW_H
#define OPTIONSWINDOW_H
#include <SDL3/SDL_video.h>


class OptionsWindow {
    public:
        static void Show();
        static void Hide();
        static void Render(SDL_Window * window);
    private:
        inline static bool visible = false;

        static void gamePathCallback(void * /*userdata*/, const char *const *filelist, int  /*filter*/);
};



#endif //OPTIONSWINDOW_H
