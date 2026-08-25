//
// Created by droc101 on 6/29/25.
//

#pragma once

#include <map>
#include <string>
#include <game_sdk/Window.h>

class AboutWindow final: public Window
{
    public:
        bool Init() override;

        void Render() override;

        const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "About the GAME SDK",
            .defaultSize = {500, 400},
            .icon = "",
            .defaultFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };

        std::string selectedComponent{};
        std::map<std::string, std::string> thirdPartyComponents{};
};
