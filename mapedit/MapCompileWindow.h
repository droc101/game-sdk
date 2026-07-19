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
        static inline bool visible = false;
        static inline SDL_Process *compilerProcess = nullptr;
        static inline SDL_IOStream *compilerOutputStream = nullptr;
        static inline SDL_IOStream *compilerErrorStream = nullptr;
        static inline bool playMap = true;
        static inline std::string gameDir{};
        static inline std::string log{};
        static inline bool outputVisible = false;
        static inline bool fastCompile = false;
        static inline bool skipLighting = false;
        static inline bool verbose = false;

        static void StartCompile();
        static void SaveLog(const std::string &path);
        static void ProcessCompilerOutput();

        static void ProcessIOStream(SDL_IOStream **stream);
        static void FinishIOSteam(SDL_IOStream **stream);
};


#endif //GAME_SDK_MAPCOMPILEWINDOW_H
