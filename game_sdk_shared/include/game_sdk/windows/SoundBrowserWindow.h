//
// Created by droc101 on 6/13/26.
//

#pragma once

#include <game_sdk/SoundSystem.h>
#include <libassets/asset/SoundAsset.h>
#include <string>
#include <vector>

class SoundBrowserWindow
{
    public:
        static SoundBrowserWindow &Get();

        void Show(std::string *sound);
        void Hide();
        void Render();

        void InputSound(const char *label, std::string &sound);

        void InputSound(const char *label, std::string *sound);

    private:
        SoundBrowserWindow() = default;

        bool visible = false;
        std::string *str = nullptr;

        std::vector<std::string> sounds;

        std::string filter;
        SoundAsset previewSoundAsset{};
        SoundSystem::Sound previewSound{};
};
