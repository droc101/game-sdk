//
// Created by droc101 on 10/20/25.
//

#pragma once

#include <string>
#include "EditorTool.h"

class AddActorTool final: public EditorTool
{
    public:
        AddActorTool() = default;
        ~AddActorTool() override = default;

        void RenderViewport(Viewport &vp) override;

        void RenderToolWindow() override;

    private:
        static inline std::string newActorType = "player";

        static inline bool hasPlacedActor = false;
        static inline glm::vec3 newActorPosition{};
};
