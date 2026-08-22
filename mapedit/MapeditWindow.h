//
// Created by droc101 on 8/21/26.
//

#pragma once

#include <game_sdk/Window.h>

#include "Viewport.h"
#include "imgui.h"

class MapeditWindow final: public Window
{
    protected:

        bool Init() override;

        void Destroy() override;

        void Render() override;

        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "GAME SDK Map Editor",
            .defaultSize = glm::ivec2(1366, 768),
            .icon = "mapedit",
            .defaultFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED,
            .defaultImguiWindow = false,
        };

        Viewport vpTopDown = Viewport(Viewport::ViewportType::TOP_DOWN_XZ);
        Viewport vpFront = Viewport(Viewport::ViewportType::FRONT_XY);
        Viewport vpSide = Viewport(Viewport::ViewportType::SIDE_YZ);

        ImGuiID dockspaceId{};
        ImGuiID rootDockspaceID{};
        bool dockspaceSetup = false;

        std::string textureBrowserToolSelection{};
        std::string materialBrowserToolSelection{};
        std::string modelBrowserToolSelection{};
        std::string soundBrowserToolSelection{};

        bool ToolbarToolButton(const char *id,
                              const char *tooltip,
                              const char *icon,
                              const bool selected,
                              const int spacing = 2,
                              const char *shortcutText = nullptr,
                              const ImGuiKeyChord shortcut = 0);

        void SaveJson(const std::string &path);

        void OpenJson(const std::string &path);

        void SetupDockspace();

};
