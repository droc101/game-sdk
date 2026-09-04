//
// Created by droc101 on 8/21/26.
//

#pragma once

#include <game_sdk/Window.h>
#include <imgui.h>
#include <SDL3/SDL_video.h>
#include <string>
#include "Viewport.h"

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

        Viewport vpPerspective = Viewport(Viewport::ViewportType::PERSPECTIVE);
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

        static bool ToolbarToolButton(const char *id,
                               const char *tooltip,
                               const char *icon,
                               bool selected,
                               int spacing = 2,
                               const char *shortcutText = nullptr,
                               ImGuiKeyChord shortcut = 0);

        static void ToolbarSeparator();

        void SaveJson(const std::string &path) const;

        void OpenJson(const std::string &path) const;

        void SetupDockspace();
};
