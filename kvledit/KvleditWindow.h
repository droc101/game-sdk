//
// Created by droc101 on 8/21/26.
//

#pragma once

#include <filesystem>
#include <game_sdk/Window.h>
#include <libassets/asset/DataAsset.h>
#include <libassets/type/Param.h>
#include <SDL3/SDL_video.h>
#include <string>

class KvleditWindow final: public Window
{
    protected:
        bool Init() override;
        void Render() override;

        const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "GAME SDK Key-Value List Editor",
            .defaultSize = {800, 600},
            .icon = "kvledit",
            .defaultFlags = SDL_WINDOW_RESIZABLE,
            .defaultImguiWindow = true,
        };

        DataAsset dataAsset{};
        std::string selectedPath = "/";
        std::string newItemName{};
        Param::ParamType newItemType = Param::ParamType::PARAM_TYPE_INTEGER;

        void OpenGkvl(const std::string &path);
        void OpenKvl(const std::string &path);
        void ImportJson(const std::string &path);

        void SaveGkvl(const std::string &path);
        void SaveKvl(const std::string &path);
        void ExportJson(const std::string &path);

        void RenderParam(Param &param,
                         const std::string &displayName,
                         const std::string &name,
                         const std::string &path);
        void RenderArray(ParamVector &vector, const std::string &path);
        void RenderKvList(KvList &list, const std::string &path);
        Param *GetSelection(const std::filesystem::path &path);
        void RenderSidebar();
};
