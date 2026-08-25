//
// Created by droc101 on 8/21/26.
//

#pragma once

#include <game_sdk/SoundSystem.h>
#include <game_sdk/Window.h>
#include <libassets/asset/SoundAsset.h>
#include <string>

class SndeditWindow final: public Window
{
    protected:
        bool Init() override;

        void Destroy() override;

        void Render() override;

        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "GAME SDK Sound Editor",
            .defaultSize = glm::ivec2(600, 280),
            .icon = "sndedit",
            .defaultFlags = 0,
            .defaultImguiWindow = true,
        };

        SoundSystem::Sound sound{};
        SoundAsset soundAsset{};

        bool LoadSound();

        void OpenGsnd(const std::string &path);

        void ImportWav(const std::string &path);

        void SaveGsnd(const std::string &path) const;

        void ExportWav(const std::string &path) const;
};
