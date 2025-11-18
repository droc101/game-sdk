//
// Created by droc101 on 11/18/25.
//

#ifndef GAME_SDK_MAPCOMPILEWINDOW_H
#define GAME_SDK_MAPCOMPILEWINDOW_H
#include <SDL3/SDL_process.h>
#include <SDL3/SDL_video.h>
#include <string>


class MapCompileWindow
{
    public:
        MapCompileWindow() = delete;

        static void Show();
        static void Render(SDL_Window *window);
    private:
        static inline bool visible = false;
        static inline SDL_Process *compilerProcess = nullptr;
        static inline bool overrideGameDir = false;
        static inline bool playMap = true;
        static inline std::string gameDir{};
        static inline std::string log{};

        static void StartCompile(SDL_Window *window);
};


#endif //GAME_SDK_MAPCOMPILEWINDOW_H
