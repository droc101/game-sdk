//
// Created by droc101 on 8/21/26.
//

#pragma once

#include <game_sdk/Window.h>

class MdleditWindow final: public Window
{
    protected:

        bool Init() override;
        void Render() override;
        const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "GAME SDK Model Editor",
            .defaultSize = glm::ivec2(1366, 768),
            .icon = "mdledit",
            .defaultFlags = SDL_WINDOW_RESIZABLE,
            .defaultImguiWindow = false,
        };

        bool openPressed = false;
        bool newPressed = false;
        bool savePressed = false;

        ImGuiID dockspaceId;
        ImGuiID rootDockspaceID;
        bool dockspaceSetup = false;

        void OpenGmdl(const std::string &path);
        void ImportModel(const std::string &path);
        void SaveGmdl(const std::string &path);

        void HandleMenuAndShortcuts();
        void SetupDockspace();
};
