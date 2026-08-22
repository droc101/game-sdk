//
// Created by droc101 on 10/21/25.
//

#pragma once

#include <game_sdk/Window.h>
#include <libassets/type/Actor.h>

class EditActorWindow final: public Window
{
    public:
        EditActorWindow(Actor &actor);

    protected:
        void Render() override;

        const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "Actor Properties",
            .defaultSize = {600, 400},
            .icon = "",
            .defaultFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };

        Actor &actor;
        size_t selectedParam = 0;
        size_t selectedConnection = 0;

        void RenderParamsTab(const ActorDefinition &definition);

        void RenderOutputsTab(const ActorDefinition &definition);
};
