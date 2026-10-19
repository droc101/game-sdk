//
// Created by droc101 on 10/19/25.
//

#pragma once

#include <SDL3/SDL_video.h>
#include <cstddef>
#include <game_sdk/Window.h>
#include <libassets/type/ActorDefinition.h>
#include <string>
#include <vector>

class ActorBrowserWindow final: public Window
{
    public:
        ActorBrowserWindow() = default;
        explicit ActorBrowserWindow(const std::string &classname);

    protected:
        void Render() override;
        [[nodiscard]] const WindowProperties &GetProperties() const override;

    private:
        WindowProperties properties = {
            .title = "Actor Class Browser",
            .defaultSize = {1000, 600},
            .icon = "",
            .defaultFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY,
            .defaultImguiWindow = true,
        };

        static constexpr std::string HOME_PAGE = "Actor Classes";

        static inline std::vector<std::string> history{};
        bool resetScrollAfterNav = false;

        void RenderHomePage();

        void RenderParamDef(const std::shared_ptr<ParamDefinition> &param);

        void RenderActorPage(const std::string &actorClass);
};
