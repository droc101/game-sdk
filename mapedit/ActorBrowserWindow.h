//
// Created by droc101 on 10/19/25.
//

#pragma once

#include <game_sdk/Window.h>
#include <libassets/type/ActorDefinition.h>
#include <libassets/type/paramDefs/ParamDefinition.h>
#include <memory>
#include <SDL3/SDL_video.h>
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

        void RenderClassTree(const std::string &baseClass);

        void RenderHomePage();

        static void RenderParamDef(const std::shared_ptr<ParamDefinition> &param);

        static void RenderParams(const ActorDefinition &def);

        static void RenderInputs(const ActorDefinition &def);

        static void RenderOutputs(const ActorDefinition &def);

        void RenderActorPage(const std::string &actorClass);
};
