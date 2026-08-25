//
// Created by droc101 on 11/12/25.
//

#pragma once

#include <game_sdk/Window.h>
#include <string>

class SetupWindow final: public Window
{
    public:

        explicit SetupWindow(bool required);

    protected:
        void Render() override;

        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "GAME SDK Setup",
            .defaultSize = {450, 250},
            .icon = "",
            .defaultFlags = SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };

        bool required = true;

        static void GamePathCallback(const std::string &path);

        static void AssetsPathCallback(const std::string &path);
};
