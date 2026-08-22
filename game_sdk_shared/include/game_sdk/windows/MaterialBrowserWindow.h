//
// Created by droc101 on 11/16/25.
//

#pragma once

#include <SDL3/SDL_video.h>
#include <game_sdk/Window.h>
#include <libassets/asset/LevelMaterialAsset.h>
#include <string>
#include <vector>

class MaterialBrowserWindow final: public Window
{
    public:
        MaterialBrowserWindow(std::string *material);

        static void InputMaterial(const char *label, std::string &material);

        static void InputMaterial(const char *label, std::string *material);

        static void Show(std::string *material);

    protected:
        bool Init() override;

        void Render() override;

        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "Choose Material",
            .defaultSize = glm::ivec2(600, 500),
            .icon = "",
            .defaultFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };

        std::string *str = nullptr;

        std::vector<std::string> materialPaths{};
        std::vector<LevelMaterialAsset> materials{};

        std::string filter;

        static constexpr int TILE_SIZE = 128;
};
