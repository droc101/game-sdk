//
// Created by droc101 on 8/20/26.
//

#pragma once

#include <game_sdk/Window.h>
#include <libassets/util/Error.h>
#include <nlohmann/json.hpp>
#include <string>

class LauncherWindow final: public Window
{
    protected:

        bool Init() override;

        void Render() override;

        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "GAME SDK",
            .defaultSize = glm::ivec2(350, 400),
            .icon = "sdk_hires",
            .defaultFlags = 0,
            .defaultImguiWindow = true,
        };

        std::string sdkPath;
        nlohmann::ordered_json launcherJson;

        std::string selectionCategory;
        std::string selectionIndex;

        Error::ErrorCode LoadLauncherConfig();

        static constexpr void StringReplace(std::string &string, const std::string &find, const std::string &replace);

        void ParsePath(std::string &path) const;

        void LaunchSelectedTool();
};
