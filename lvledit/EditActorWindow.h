//
// Created by droc101 on 10/21/25.
//

#pragma once
#include <cstddef>
#include <libassets/type/Actor.h>

class EditActorWindow
{
    public:
        EditActorWindow() = delete;

        static inline bool visible = false;
        static inline size_t selectedParam = 0;
        static inline size_t selectedConnection = 0;

        static void Render(Actor &actor);

    private:
        static void RenderParamsTab(Actor &actor, const ActorDefinition &definition);

        static void RenderOutputsTab(Actor &actor, const ActorDefinition &definition);
};
