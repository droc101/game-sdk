//
// Created by droc101 on 11/17/25.
//

#pragma once

#include <SDL3/SDL_video.h>
#include <game_sdk/Window.h>

class MapPropertiesWindow final: public Window
{
    protected:
        void Render() override;
        [[nodiscard]] const WindowProperties &GetProperties() const override;
    private:
        WindowProperties properties = {
            .title = "Map Properties",
            .defaultSize = glm::ivec2(400, 250),
            .icon = "",
            .defaultFlags = SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };
};
