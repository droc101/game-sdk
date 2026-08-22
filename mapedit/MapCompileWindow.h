//
// Created by droc101 on 11/18/25.
//

#ifndef GAME_SDK_MAPCOMPILEWINDOW_H
#define GAME_SDK_MAPCOMPILEWINDOW_H

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_process.h>
#include <string>

#include "CompileProgressWindow.h"

class MapCompileWindow final: public Window
{
    protected:
        void Render() override;
        const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "Compile Map",
            .defaultSize = glm::ivec2(250, 300),
            .icon = "",
            .defaultFlags = SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };

        CompileProgressWindow::CompileOptions opts;

        void StartCompile();
};


#endif //GAME_SDK_MAPCOMPILEWINDOW_H
