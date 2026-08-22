//
// Created by droc101 on 7/6/25.
//

#pragma once

#include <SDL3/SDL_video.h>
#include <game_sdk/Window.h>
#include <string>
#include <vector>

class TextureBrowserWindow final: public Window
{
    public:
        TextureBrowserWindow(std::string *texture);

        static void InputTexture(const char *label, std::string &texture);

        static void InputTexture(const char *label, std::string *texture);

        static void Show(std::string *texture);

    protected:
        bool Init() override;
        void Render() override;
        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "Choose Texture",
            .defaultSize = glm::ivec2(600, 500),
            .icon = "",
            .defaultFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };

        std::string *str = nullptr;

        std::vector<std::string> textures;

        std::string filter;

        static constexpr int TILE_SIZE = 128;
};
