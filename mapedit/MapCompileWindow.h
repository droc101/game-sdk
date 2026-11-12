//
// Created by droc101 on 11/18/25.
//

#ifndef GAME_SDK_MAPCOMPILEWINDOW_H
#define GAME_SDK_MAPCOMPILEWINDOW_H

#include <game_sdk/Window.h>
#include "CompileProgressWindow.h"

class MapCompileWindow final: public Window
{
    protected:
        void Render() override;
        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "Compile Map",
            .defaultSize = glm::ivec2(250, 290),
            .icon = "",
            .defaultFlags = 0,
            .defaultImguiWindow = true,
        };

        CompileProgressWindow::CompileOptions opts;

        void StartCompile();
};


#endif //GAME_SDK_MAPCOMPILEWINDOW_H
