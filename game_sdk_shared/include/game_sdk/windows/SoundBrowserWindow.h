//
// Created by droc101 on 6/13/26.
//

#pragma once

#include <game_sdk/SoundSystem.h>
#include <game_sdk/Window.h>
#include <libassets/asset/SoundAsset.h>
#include <string>
#include <vector>

class SoundBrowserWindow final: public Window
{
    public:
        SoundBrowserWindow(std::string *sound);

        static void Show(std::string *sound);

        static void InputSound(const char *label, std::string &sound);

        static void InputSound(const char *label, std::string *sound);

    protected:
        bool Init() override;
        void Destroy() override;
        void Render() override;

    private:
        WindowProperties properties = {
            .title = "Choose Sound",
            .defaultSize = glm::ivec2(600, 500),
            .icon = "",
            .defaultFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };

        std::string *str = nullptr;

        std::vector<std::string> sounds;

        std::string filter;
        SoundAsset previewSoundAsset{};
        SoundSystem::Sound previewSound{};
};
