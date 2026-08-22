//
// Created by droc101 on 8/21/26.
//

#pragma once

#include <game_sdk/Window.h>
#include <libassets/asset/LevelMaterialAsset.h>
#include <string>

class MtleditWindow final: public Window
{
    protected:
        bool Init() override;
        void Render() override;
        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "GAME SDK Material Editor",
            .defaultSize = glm::ivec2(400, 200),
            .icon = "mtledit",
            .defaultFlags = 0,
            .defaultImguiWindow = true,
        };

        void OpenGmtl(const std::string &path);
        void SaveGmtl(const std::string &path) const;

        LevelMaterialAsset material{};
};
