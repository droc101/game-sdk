//
// Created by droc101 on 10/29/26.
//

#pragma once

#include <SDL3/SDL_video.h>
#include <game_sdk/Window.h>

class MapeditOptionsWindow final: public Window
{
    protected:
        void Render() override;

        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "Map Editor Options",
            .defaultSize = glm::ivec2(400, 450),
            .icon = "",
            .defaultFlags = SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };
};
