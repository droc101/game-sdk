//
// Created by droc101 on 8/21/26.
//

#pragma once

#include <SDL3/SDL_video.h>
#include <game_sdk/Window.h>
#include <libassets/asset/ShaderAsset.h>
#include <libassets/util/Error.h>
#include <string>
#include <vector>

class ShdeditWindow final: public Window
{
    protected:
        void Render() override;
        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "GAME SDK Shader Editor",
            .defaultSize = glm::ivec2(600, 600),
            .icon = "shdedit",
            .defaultFlags = SDL_WINDOW_RESIZABLE,
            .defaultImguiWindow = true,
        };

        std::vector<std::string> files{};
        std::vector<ShaderAsset::ShaderType> types{};

        std::string outputFolder{};
        bool replicateFolderStructure = false;
        std::string sourcesBaseFolder{};
        bool enableOptimization = false;
        bool dumpBinaries = false;

        void SelectCallback(const std::vector<std::string> &paths);
        void OutPathCallback(const std::string &path);
        void BasePathCallback(const std::string &path);
        void AddFolderCallback(const std::string &folder);
        Error::ErrorCode Execute(std::string &errorLog);
};
