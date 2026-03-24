//
// Created by droc101 on 11/18/25.
//

#ifndef GAME_SDK_MAPCOMPILEWINDOW_H
#define GAME_SDK_MAPCOMPILEWINDOW_H

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_process.h>
#include <string>

class MapCompileWindow
{
    public:
        MapCompileWindow() = delete;

        static void Show();
        static void Render();
        static void RenderCompileOutput();

    private:
        enum class LightingCompileMode: uint8_t
        {
            FULL_COMPILE,
            FAST_COMPILE,
            DONT_COMPILE
        };

        static inline bool visible = false;
        static inline SDL_Process *compilerProcess = nullptr;
        static inline SDL_IOStream *compilerOutputStream = nullptr;
        static inline bool playMap = true;
        static inline bool bakeOnCpu = false;
        static inline std::string gameDir{};
        static inline std::string log{};
        static inline bool outputVisible = false;
        static inline LightingCompileMode lightCompileMode = LightingCompileMode::FULL_COMPILE;

        static void StartCompile();
        static void SaveLog(const std::string &path);
        static void ProcessCompilerOutput();
};


#endif //GAME_SDK_MAPCOMPILEWINDOW_H
