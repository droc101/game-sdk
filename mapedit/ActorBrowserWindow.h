//
// Created by droc101 on 10/19/25.
//

#pragma once

#include <SDL3/SDL_video.h>
#include <cstddef>
#include <game_sdk/Window.h>
#include <libassets/type/ActorDefinition.h>
#include <string>

class ActorBrowserWindow final: public Window
{
    protected:
        void Render() override;
        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "Actor Class Browser",
            .defaultSize = {640, 480},
            .icon = "",
            .defaultFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };

        std::string selectedClass = "actor";
        size_t selectedParam = 0;

        void RenderParamsTab(const ActorDefinition &def);

        void RenderInputsTab(const ActorDefinition &def);

        void RenderOutputsTab(const ActorDefinition &def);
};
