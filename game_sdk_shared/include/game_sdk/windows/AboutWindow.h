//
// Created by droc101 on 6/29/25.
//

#pragma once

#include <game_sdk/Window.h>
#include <map>
#include <SDL3/SDL_video.h>
#include <string>

class AboutWindow final: public Window
{
    protected:
        bool Init() override;

        void Render() override;

        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "About the GAME SDK",
            .defaultSize = {900, 600},
            .icon = "",
            .defaultFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };

        std::string selectedComponent{};
        std::map<std::string, std::string> thirdPartyComponents{};
};
