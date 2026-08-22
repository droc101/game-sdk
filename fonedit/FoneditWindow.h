//
// Created by droc101 on 8/21/26.
//

#pragma once

#include <game_sdk/Window.h>
#include <libassets/asset/FontAsset.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <vector>

class FoneditWindow final: public Window
{
    protected:
        bool Init() override;
        void Render() override;
        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "GAME SDK Font Editor",
            .defaultSize = {800, 600},
            .icon = "fonedit",
            .defaultFlags = SDL_WINDOW_RESIZABLE,
            .defaultImguiWindow = true,
        };

        std::vector<std::string> charDisplayList{};
        FontAsset font{};

        void OpenGfon(const std::string &path);
        void SaveGfon(const std::string &path);
        static bool ComboGetter(void *data, int index, const char **outText);
};
