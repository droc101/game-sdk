//
// Created by droc101 on 8/21/26.
//

#pragma once

#include <game_sdk/Window.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_process.h>
#include <string>

class CompileProgressWindow final: public Window
{
    public:
        struct CompileOptions
        {
                bool fastCompile = false;
                bool skipLighting = false;
                bool verbose = false;
                bool playMap = true;
                std::string gameDir{};
        };

        CompileProgressWindow(const CompileOptions &opts);

    protected:
        bool Init() override;
        void Render() override;
        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "Compile Progress",
            .defaultSize = glm::ivec2(900, 640),
            .icon = "",
            .defaultFlags = SDL_WINDOW_UTILITY | SDL_WINDOW_RESIZABLE,
            .defaultImguiWindow = true,
        };

        SDL_Process *compilerProcess = nullptr;
        SDL_IOStream *compilerOutputStream = nullptr;
        SDL_IOStream *compilerErrorStream = nullptr;
        std::string log{};
        CompileOptions opts{};

        void SaveLog(const std::string &path) const;
        void ProcessCompilerOutput();

        void ProcessIOStream(SDL_IOStream **stream);
        void FinishIOSteam(SDL_IOStream **stream);
};
