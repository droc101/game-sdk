//
// Created by droc101 on 6/29/25.
//

#pragma once

#include <game_sdk/Window.h>
#include <SDL3/SDL_video.h>
#include <string>

class OptionsWindow final: public Window
{
    protected:
        void Render() override;

        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "GAME SDK Setup",
            .defaultSize = {450, 320},
            .icon = "",
            .defaultFlags = SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };

        static void GamePathCallback(const std::string &path);

        static void AssetsPathCallback(const std::string &path);
};
